// Borealis Native Horizon GPU Graphical UI for 3DS to ZCCI Converter (Nintendo Switch)
#include "borealis_gui.h"
#include <switch.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <thread>
#include <chrono>
#include <ctime>
#include <algorithm>

namespace SwitchZCCI {

std::string FormatSize(uint64_t bytes) {
    char buf[64];
    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        snprintf(buf, sizeof(buf), "%.2f GB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    } else if (bytes >= 1024ULL * 1024ULL) {
        snprintf(buf, sizeof(buf), "%.2f MB", (double)bytes / (1024.0 * 1024.0));
    } else if (bytes >= 1024ULL) {
        snprintf(buf, sizeof(buf), "%.2f KB", (double)bytes / 1024.0);
    } else {
        snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)bytes);
    }
    return std::string(buf);
}

static std::string FormatETA(double remaining_seconds) {
    if (remaining_seconds <= 0.0 || remaining_seconds > 86400.0) return "calculando...";
    int total_sec = (int)remaining_seconds;
    int minutes = total_sec / 60;
    int seconds = total_sec % 60;
    char buf[64];
    if (minutes > 0) {
        snprintf(buf, sizeof(buf), "%02dm %02ds", minutes, seconds);
    } else {
        snprintf(buf, sizeof(buf), "%02ds", seconds);
    }
    return std::string(buf);
}

static std::vector<GameFile> ScanDirectory(const char* folder) {
    std::vector<GameFile> files;
    DIR* dir = opendir(folder);
    if (!dir) return files;

    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_REG) {
            size_t len = strlen(ent->d_name);
            if ((len > 4 && strcasecmp(ent->d_name + len - 4, ".3ds") == 0) ||
                (len > 4 && strcasecmp(ent->d_name + len - 4, ".cci") == 0)) {
                
                std::string full_path = std::string(folder) + "/" + std::string(ent->d_name);
                struct stat st;
                uint64_t fsize = 0;
                if (stat(full_path.c_str(), &st) == 0) {
                    fsize = st.st_size;
                }

                files.push_back({full_path, std::string(ent->d_name), fsize});
            }
        }
    }
    closedir(dir);
    return files;
}

static std::string GetSystemTimeString() {
    time_t rawtime = time(NULL);
    struct tm* timeinfo = localtime(&rawtime);
    char buf[16];
    if (timeinfo) {
        strftime(buf, sizeof(buf), "%H:%M", timeinfo);
        return std::string(buf);
    }
    return "--:--";
}

static std::string GetSystemTempString() {
    s32 temp_val = 0;
    if (R_SUCCEEDED(tsInitialize())) {
        if (R_SUCCEEDED(tsGetTemperatureMilliC(TsLocation_External, &temp_val))) {
            tsExit();
            float temp_c = (float)temp_val / 1000.0f;
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f C", temp_c);
            return std::string(buf);
        } else if (R_SUCCEEDED(tsGetTemperature(TsLocation_External, &temp_val))) {
            tsExit();
            float temp_c = (temp_val > 1000) ? ((float)temp_val / 1000.0f) : (float)temp_val;
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f C", temp_c);
            return std::string(buf);
        } else if (R_SUCCEEDED(tsGetTemperatureMilliC(TsLocation_Internal, &temp_val))) {
            tsExit();
            float temp_c = (float)temp_val / 1000.0f;
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f C", temp_c);
            return std::string(buf);
        }
        tsExit();
    }
    
    if (R_SUCCEEDED(tcInitialize())) {
        if (R_SUCCEEDED(tcGetSkinTemperatureMilliC(&temp_val))) {
            tcExit();
            float temp_c = (float)temp_val / 1000.0f;
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f C", temp_c);
            return std::string(buf);
        }
        tcExit();
    }
    
    return "42.0 C";
}

static std::string GetSystemBatteryString() {
    u32 pct = 0;
    PsmChargerType charger = PsmChargerType_Unconnected;

    if (R_SUCCEEDED(psmInitialize())) {
        psmGetBatteryChargePercentage(&pct);
        psmGetChargerType(&charger);
        psmExit();

        char buf[32];
        if (charger != PsmChargerType_Unconnected) {
            snprintf(buf, sizeof(buf), "%u%% \x0B", pct); // \x0B = Lightning Bolt Glyph
        } else {
            snprintf(buf, sizeof(buf), "%u%%", pct);
        }
        return std::string(buf);
    }
    return "100%";
}

static const uint8_t* GetGlyph(char c) {
    static const uint8_t g_heart[8]     = {0x00,0x66,0xFF,0xFF,0x7E,0x3C,0x18,0x00}; // \x03 = Heart
    static const uint8_t g_a_acc[8]     = {0x0C,0x00,0x3E,0x03,0x3F,0x63,0x3F,0x00}; // \x01 = á
    static const uint8_t g_e_acc[8]     = {0x0C,0x00,0x3E,0x63,0x7F,0x60,0x3E,0x00}; // \x02 = é
    static const uint8_t g_i_acc[8]     = {0x0C,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}; // \x04 = í
    static const uint8_t g_o_acc[8]     = {0x0C,0x00,0x3E,0x63,0x63,0x63,0x3E,0x00}; // \x05 = ó
    static const uint8_t g_u_acc[8]     = {0x0C,0x00,0x63,0x63,0x63,0x63,0x3F,0x00}; // \x06 = ú
    static const uint8_t g_n_tilde[8]   = {0x36,0x00,0x7E,0x63,0x63,0x63,0x63,0x00}; // \x07 = ñ
    static const uint8_t g_inv_quest[8]  = {0x00,0x18,0x00,0x18,0x30,0x60,0x7C,0x00}; // \x0A = ¿
    static const uint8_t g_lightning[8]  = {0x0C,0x18,0x30,0x7E,0x0C,0x18,0x30,0x00}; // \x0B = Lightning Bolt

    static const uint8_t g_0[8] = {0x3E,0x63,0x73,0x6B,0x6F,0x67,0x3E,0x00};
    static const uint8_t g_1[8] = {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3E,0x00};
    static const uint8_t g_2[8] = {0x3E,0x63,0x07,0x1E,0x30,0x60,0x7F,0x00};
    static const uint8_t g_3[8] = {0x3E,0x63,0x07,0x1C,0x07,0x63,0x3E,0x00};
    static const uint8_t g_4[8] = {0x0E,0x1E,0x36,0x66,0x7F,0x06,0x0F,0x00};
    static const uint8_t g_5[8] = {0x7F,0x60,0x7E,0x03,0x03,0x63,0x3E,0x00};
    static const uint8_t g_6[8] = {0x1C,0x30,0x60,0x7E,0x63,0x63,0x3E,0x00};
    static const uint8_t g_7[8] = {0x7F,0x63,0x06,0x0C,0x18,0x18,0x18,0x00};
    static const uint8_t g_8[8] = {0x3E,0x63,0x63,0x3E,0x63,0x63,0x3E,0x00};
    static const uint8_t g_9[8] = {0x3E,0x63,0x63,0x3F,0x03,0x06,0x3C,0x00};
    static const uint8_t g_A[8] = {0x1C,0x36,0x63,0x63,0x7F,0x63,0x63,0x00};
    static const uint8_t g_B[8] = {0x7E,0x63,0x63,0x7E,0x63,0x63,0x7E,0x00};
    static const uint8_t g_C[8] = {0x3E,0x63,0x60,0x60,0x60,0x63,0x3E,0x00};
    static const uint8_t g_D[8] = {0x7C,0x66,0x63,0x63,0x63,0x66,0x7C,0x00};
    static const uint8_t g_E[8] = {0x7F,0x60,0x60,0x78,0x60,0x60,0x7F,0x00};
    static const uint8_t g_F[8] = {0x7F,0x60,0x60,0x78,0x60,0x60,0x60,0x00};
    static const uint8_t g_G[8] = {0x3E,0x63,0x60,0x6F,0x63,0x63,0x3E,0x00};
    static const uint8_t g_H[8] = {0x63,0x63,0x63,0x7F,0x63,0x63,0x63,0x00};
    static const uint8_t g_I[8] = {0x3E,0x0C,0x0C,0x0C,0x0C,0x0C,0x3E,0x00};
    static const uint8_t g_J[8] = {0x1F,0x06,0x06,0x06,0x06,0x66,0x3C,0x00};
    static const uint8_t g_K[8] = {0x67,0x66,0x6C,0x78,0x6C,0x66,0x67,0x00};
    static const uint8_t g_L[8] = {0x60,0x60,0x60,0x60,0x60,0x60,0x7F,0x00};
    static const uint8_t g_M[8] = {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00};
    static const uint8_t g_N[8] = {0x63,0x73,0x7B,0x6F,0x67,0x63,0x63,0x00};
    static const uint8_t g_O[8] = {0x3E,0x63,0x63,0x63,0x63,0x63,0x3E,0x00};
    static const uint8_t g_P[8] = {0x7E,0x63,0x63,0x7E,0x60,0x60,0x60,0x00};
    static const uint8_t g_Q[8] = {0x3E,0x63,0x63,0x63,0x6B,0x67,0x3E,0x0E};
    static const uint8_t g_R[8] = {0x7E,0x63,0x63,0x7E,0x6C,0x66,0x63,0x00};
    static const uint8_t g_S[8] = {0x3E,0x63,0x60,0x3E,0x03,0x63,0x3E,0x00};
    static const uint8_t g_T[8] = {0x7F,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x00};
    static const uint8_t g_U[8] = {0x63,0x63,0x63,0x63,0x63,0x63,0x3E,0x00};
    static const uint8_t g_V[8] = {0x63,0x63,0x63,0x63,0x36,0x1C,0x08,0x00};
    static const uint8_t g_W[8] = {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00};
    static const uint8_t g_X[8] = {0x63,0x63,0x36,0x1C,0x36,0x63,0x63,0x00};
    static const uint8_t g_Y[8] = {0x63,0x63,0x36,0x1C,0x1C,0x1C,0x1C,0x00};
    static const uint8_t g_Z[8] = {0x7F,0x03,0x06,0x1C,0x30,0x60,0x7F,0x00};
    static const uint8_t g_a[8] = {0x00,0x00,0x3E,0x03,0x3F,0x63,0x3F,0x00};
    static const uint8_t g_b[8] = {0x60,0x60,0x7E,0x63,0x63,0x63,0x7E,0x00};
    static const uint8_t g_c[8] = {0x00,0x00,0x3E,0x63,0x60,0x63,0x3E,0x00};
    static const uint8_t g_d[8] = {0x03,0x03,0x3F,0x63,0x63,0x63,0x3F,0x00};
    static const uint8_t g_e[8] = {0x00,0x00,0x3E,0x63,0x7F,0x60,0x3E,0x00};
    static const uint8_t g_f[8] = {0x0E,0x18,0x3C,0x18,0x18,0x18,0x18,0x00};
    static const uint8_t g_g[8] = {0x00,0x00,0x3F,0x63,0x63,0x3F,0x03,0x3E};
    static const uint8_t g_h[8] = {0x60,0x60,0x7E,0x63,0x63,0x63,0x63,0x00};
    static const uint8_t g_i[8] = {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00};
    static const uint8_t g_j[8] = {0x06,0x00,0x0E,0x06,0x06,0x66,0x3C,0x00};
    static const uint8_t g_k[8] = {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00};
    static const uint8_t g_l[8] = {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00};
    static const uint8_t g_m[8] = {0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00};
    static const uint8_t g_n[8] = {0x00,0x00,0x7E,0x63,0x63,0x63,0x63,0x00};
    static const uint8_t g_o[8] = {0x00,0x00,0x3E,0x63,0x63,0x63,0x3E,0x00};
    static const uint8_t g_p[8] = {0x00,0x00,0x7E,0x63,0x63,0x7E,0x60,0x60};
    static const uint8_t g_q[8] = {0x00,0x00,0x3F,0x63,0x63,0x3F,0x03,0x03};
    static const uint8_t g_r[8] = {0x00,0x00,0x7E,0x63,0x60,0x60,0x60,0x00};
    static const uint8_t g_s[8] = {0x00,0x00,0x3E,0x60,0x3E,0x03,0x7E,0x00};
    static const uint8_t g_t[8] = {0x18,0x18,0x7E,0x18,0x18,0x18,0x0E,0x00};
    static const uint8_t g_u[8] = {0x00,0x00,0x63,0x63,0x63,0x63,0x3F,0x00};
    static const uint8_t g_v[8] = {0x00,0x00,0x63,0x63,0x63,0x36,0x1C,0x00};
    static const uint8_t g_w[8] = {0x00,0x00,0x63,0x63,0x6B,0x7F,0x36,0x00};
    static const uint8_t g_x[8] = {0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00};
    static const uint8_t g_y[8] = {0x00,0x00,0x63,0x63,0x63,0x3F,0x03,0x3E};
    static const uint8_t g_z[8] = {0x00,0x00,0x7F,0x06,0x0C,0x18,0x7F,0x00};
    static const uint8_t g_colon[8] = {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00};
    static const uint8_t g_dot[8] = {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00};
    static const uint8_t g_dash[8] = {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00};
    static const uint8_t g_plus[8] = {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00};
    static const uint8_t g_slash[8] = {0x00,0x03,0x06,0x0C,0x18,0x30,0x60,0x00};
    static const uint8_t g_lparen[8] = {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00};
    static const uint8_t g_rparen[8] = {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00};
    static const uint8_t g_lbracket[8] = {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00};
    static const uint8_t g_rbracket[8] = {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00};
    static const uint8_t g_lt[8] = {0x06,0x1C,0x70,0x60,0x70,0x1C,0x06,0x00};
    static const uint8_t g_gt[8] = {0x60,0x70,0x1C,0x06,0x1C,0x70,0x60,0x00};
    static const uint8_t g_pct[8] = {0x61,0x63,0x06,0x0C,0x18,0x63,0x43,0x00};
    static const uint8_t g_eq[8] = {0x00,0x7E,0x00,0x7E,0x00,0x00,0x00,0x00};
    static const uint8_t g_excl[8] = {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00};
    static const uint8_t g_quest[8] = {0x3E,0x63,0x06,0x0C,0x18,0x00,0x18,0x00};
    static const uint8_t g_pipe[8] = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00};
    static const uint8_t g_space[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    switch (c) {
        case '\x01': return g_a_acc;
        case '\x02': return g_e_acc;
        case '\x03': return g_heart;
        case '\x04': return g_i_acc;
        case '\x05': return g_o_acc;
        case '\x06': return g_u_acc;
        case '\x07': return g_n_tilde;
        case '\x0A': return g_inv_quest;
        case '\x0B': return g_lightning;
        case '0': return g_0; case '1': return g_1; case '2': return g_2; case '3': return g_3;
        case '4': return g_4; case '5': return g_5; case '6': return g_6; case '7': return g_7;
        case '8': return g_8; case '9': return g_9;
        case 'A': return g_A; case 'B': return g_B; case 'C': return g_C; case 'D': return g_D;
        case 'E': return g_E; case 'F': return g_F; case 'G': return g_G; case 'H': return g_H;
        case 'I': return g_I; case 'J': return g_J; case 'K': return g_K; case 'L': return g_L;
        case 'M': return g_M; case 'N': return g_N; case 'O': return g_O; case 'P': return g_P;
        case 'Q': return g_Q; case 'R': return g_R; case 'S': return g_S; case 'T': return g_T;
        case 'U': return g_U; case 'V': return g_V; case 'W': return g_W; case 'X': return g_X;
        case 'Y': return g_Y; case 'Z': return g_Z;
        case 'a': return g_a; case 'b': return g_b; case 'c': return g_c; case 'd': return g_d;
        case 'e': return g_e; case 'f': return g_f; case 'g': return g_g; case 'h': return g_h;
        case 'i': return g_i; case 'j': return g_j; case 'k': return g_k; case 'l': return g_l;
        case 'm': return g_m; case 'n': return g_n; case 'o': return g_o; case 'p': return g_p;
        case 'q': return g_q; case 'r': return g_r; case 's': return g_s; case 't': return g_t;
        case 'u': return g_u; case 'v': return g_v; case 'w': return g_w; case 'x': return g_x;
        case 'y': return g_y; case 'z': return g_z;
        case ':': return g_colon; case '.': return g_dot; case '-': return g_dash; case '+': return g_plus;
        case '/': return g_slash; case '(': return g_lparen; case ')': return g_rparen; case '[': return g_lbracket;
        case ']': return g_rbracket; case '<': return g_lt; case '>': return g_gt; case '%': return g_pct;
        case '=': return g_eq; case '!': return g_excl; case '?': return g_quest; case '|': return g_pipe;
        default: return g_space;
    }
}

static void DrawChar(SDL_Renderer* renderer, char c, int x, int y, SDL_Color color, int scale = 2) {
    if (c == '\x03') {
        SDL_SetRenderDrawColor(renderer, 239, 68, 68, 255); // Red heart
    } else if (c == '\x0B') {
        SDL_SetRenderDrawColor(renderer, 252, 211, 77, 255); // Yellow lightning bolt
    } else {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }
    const uint8_t* glyph = GetGlyph(c);

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (glyph[row] & (1 << (7 - col))) {
                SDL_Rect r = { x + col * scale, y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }
}

static void DrawString(SDL_Renderer* renderer, const char* str, int x, int y, SDL_Color color, int scale = 2) {
    int cur_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 9 * scale;
            cur_x = x;
        } else {
            DrawChar(renderer, *str, cur_x, y, color, scale);
            cur_x += 8 * scale;
        }
        str++;
    }
}

static void DrawChileFlag(SDL_Renderer* renderer, int x, int y) {
    SDL_Rect white_top = { x, y, 24, 8 };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &white_top);

    SDL_Rect red_bottom = { x, y + 8, 24, 8 };
    SDL_SetRenderDrawColor(renderer, 220, 38, 38, 255);
    SDL_RenderFillRect(renderer, &red_bottom);

    SDL_Rect blue_sq = { x, y, 8, 8 };
    SDL_SetRenderDrawColor(renderer, 29, 78, 216, 255);
    SDL_RenderFillRect(renderer, &blue_sq);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect star = { x + 3, y + 3, 2, 2 };
    SDL_RenderFillRect(renderer, &star);
}

static void DrawMagicWand(SDL_Renderer* renderer, int x, int y) {
    SDL_SetRenderDrawColor(renderer, 217, 119, 6, 255);
    for (int i = 0; i < 10; ++i) {
        SDL_Rect r = { x + i, y + 14 - i, 2, 2 };
        SDL_RenderFillRect(renderer, &r);
    }
    SDL_SetRenderDrawColor(renderer, 253, 224, 71, 255);
    SDL_Rect tip = { x + 9, y + 3, 4, 4 };
    SDL_RenderFillRect(renderer, &tip);

    SDL_SetRenderDrawColor(renderer, 250, 204, 21, 255);
    SDL_Rect s1 = { x + 15, y + 1, 2, 2 };
    SDL_Rect s2 = { x + 6, y + 1, 2, 2 };
    SDL_Rect s3 = { x + 14, y + 8, 2, 2 };
    SDL_RenderFillRect(renderer, &s1);
    SDL_RenderFillRect(renderer, &s2);
    SDL_RenderFillRect(renderer, &s3);
}

void RunBorealisGUI() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        consoleInit(NULL);
        printf("Error initializing SDL2 Video\n");
        return;
    }

    SDL_Window* window = SDL_CreateWindow(
        "3DS to ZCCI Converter",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720, 0
    );

    if (!window) {
        SDL_Quit();
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    const std::vector<std::string> paths = {
        "sdmc:/tico/roms/3ds",
        "sdmc:/roms/3ds",
        "sdmc:/3ds",
        "sdmc:"
    };

    std::vector<GameFile> games;
    for (const auto& p : paths) {
        auto found = ScanDirectory(p.c_str());
        games.insert(games.end(), found.begin(), found.end());
    }

    int active_tab_idx = 0; // 0 = Games, 1 = About
    int selected_idx = 0;
    bool running = true;
    bool prev_touch_down = false;

    SDL_Color c_bg = {24, 24, 24, 255};
    SDL_Color c_header_bg = {31, 41, 55, 255};
    SDL_Color c_accent_green = {16, 185, 129, 255};
    SDL_Color c_card_bg = {38, 38, 38, 255};
    SDL_Color c_card_selected = {16, 185, 129, 255};
    SDL_Color c_text_white = {255, 255, 255, 255};
    SDL_Color c_text_yellow = {252, 211, 77, 255};
    SDL_Color c_text_gray = {156, 163, 175, 255};
    SDL_Color c_text_black = {0, 0, 0, 255};

    auto RenderFullBackground = [&]() {
        SDL_SetRenderDrawColor(renderer, c_bg.r, c_bg.g, c_bg.b, 255);
        SDL_RenderClear(renderer);

        // 1. Top Header Bar
        SDL_Rect header_rect = { 0, 0, 1280, 80 };
        SDL_SetRenderDrawColor(renderer, c_header_bg.r, c_header_bg.g, c_header_bg.b, 255);
        SDL_RenderFillRect(renderer, &header_rect);

        SDL_Rect line_rect = { 0, 80, 1280, 4 };
        SDL_SetRenderDrawColor(renderer, c_accent_green.r, c_accent_green.g, c_accent_green.b, 255);
        SDL_RenderFillRect(renderer, &line_rect);

        DrawString(renderer, "3DS to ZCCI Converter", 30, 20, c_text_yellow, 3);
        DrawString(renderer, "v1.1.4 by elcoke.cl", 30, 52, c_accent_green, 2);

        std::string info_top = GetSystemTimeString() + "  |  " + GetSystemTempString() + "  |  " + GetSystemBatteryString();
        DrawString(renderer, info_top.c_str(), 840, 30, c_text_white, 2);

        // 2. Navigation Tabs
        SDL_Rect tab1_rect = { 40, 100, 220, 40 };
        SDL_SetRenderDrawColor(renderer, (active_tab_idx == 0 ? c_accent_green.r : 50),
                                         (active_tab_idx == 0 ? c_accent_green.g : 50),
                                         (active_tab_idx == 0 ? c_accent_green.b : 50), 255);
        SDL_RenderFillRect(renderer, &tab1_rect);
        DrawString(renderer, "ROMs 3DS", 60, 110, (active_tab_idx == 0 ? c_text_black : c_text_white), 2);

        SDL_Rect tab2_rect = { 280, 100, 220, 40 };
        SDL_SetRenderDrawColor(renderer, (active_tab_idx == 1 ? c_accent_green.r : 50),
                                         (active_tab_idx == 1 ? c_accent_green.g : 50),
                                         (active_tab_idx == 1 ? c_accent_green.b : 50), 255);
        SDL_RenderFillRect(renderer, &tab2_rect);
        DrawString(renderer, "Acerca de", 300, 110, (active_tab_idx == 1 ? c_text_black : c_text_white), 2);

        if (active_tab_idx == 0) {
            if (games.empty()) {
                DrawString(renderer, "No se encontraron ROMs .3ds en la tarjeta SD.", 60, 200, c_text_white, 2);
            } else {
                int max_visible = 6;
                int start_idx = 0;
                if (selected_idx >= max_visible) {
                    start_idx = selected_idx - max_visible + 1;
                }

                int card_y = 160;
                for (int i = start_idx; i < (int)games.size() && i < start_idx + max_visible; ++i) {
                    bool is_sel = (i == selected_idx);

                    SDL_Rect card_r = { 40, card_y, 1200, 70 };
                    SDL_SetRenderDrawColor(renderer, (is_sel ? c_card_selected.r : c_card_bg.r),
                                                     (is_sel ? c_card_selected.g : c_card_bg.g),
                                                     (is_sel ? c_card_selected.b : c_card_bg.b), 255);
                    SDL_RenderFillRect(renderer, &card_r);

                    SDL_Rect badge_r = { 55, card_y + 15, 80, 40 };
                    SDL_SetRenderDrawColor(renderer, 220, 38, 38, 255);
                    SDL_RenderFillRect(renderer, &badge_r);
                    DrawString(renderer, "3DS", 70, card_y + 25, c_text_white, 2);

                    DrawString(renderer, games[i].filename.c_str(), 155, card_y + 15, (is_sel ? c_text_black : c_text_white), 2);

                    std::string sz_str = FormatSize(games[i].size_bytes);
                    DrawString(renderer, sz_str.c_str(), 1050, card_y + 25, (is_sel ? c_text_black : c_text_yellow), 2);

                    card_y += 80;
                }
            }
        } else {
            DrawString(renderer, "Developed by: elcoke.cl", 60, 180, c_text_yellow, 3);
            DrawString(renderer, "Versi\x05n: v1.1.4", 60, 230, c_accent_green, 2);

            DrawString(renderer, "Agradecimientos Especiales:", 60, 290, c_text_white, 3);
            DrawString(renderer, "- A toda la scene homebrew de Nintendo Switch", 80, 340, c_text_gray, 2);
            DrawString(renderer, "- A los creadores del emulador Azahar y Citra Project", 80, 370, c_text_gray, 2);
            DrawString(renderer, "- Al equipo de devkitPro y libnx", 80, 400, c_text_gray, 2);
            DrawString(renderer, "- A los creadores del framework Borealis", 80, 430, c_text_gray, 2);
            DrawString(renderer, "- Y no menos importante, a mi esposa Bel\x02n y mi hija Amalia", 80, 470, c_text_yellow, 2);
            DrawString(renderer, "  por ser las luces de mi vida \x03", 80, 500, c_text_yellow, 2);

            DrawString(renderer, "Desde CholChol - Chile", 60, 580, c_accent_green, 2);
            DrawChileFlag(renderer, 415, 584);

            DrawString(renderer, "con amor para los amantes de la magia", 450, 580, c_accent_green, 2);
            DrawMagicWand(renderer, 1045, 580);
        }

        DrawString(renderer, "[Up/Down] Seleccionar   [L/R] Pestanas   [A] Convertir   [+] Salir", 140, 680, c_text_gray, 2);
    };

    while (running && appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        HidTouchScreenState touch_st;
        hidGetTouchScreenStates(&touch_st, 1);
        bool touch_down = (touch_st.count > 0);
        bool touch_tap = (touch_down && !prev_touch_down);
        prev_touch_down = touch_down;

        int touch_x = touch_down ? touch_st.touches[0].x : 0;
        int touch_y = touch_down ? touch_st.touches[0].y : 0;

        if (kDown & HidNpadButton_Plus) {
            running = false;
        }

        if ((kDown & HidNpadButton_L) || (touch_tap && touch_y >= 90 && touch_y <= 150 && touch_x >= 40 && touch_x <= 260)) {
            active_tab_idx = 0;
        }
        if ((kDown & HidNpadButton_R) || (touch_tap && touch_y >= 90 && touch_y <= 150 && touch_x >= 280 && touch_x <= 500)) {
            active_tab_idx = 1;
        }

        bool trigger_conversion = false;

        if (active_tab_idx == 0 && !games.empty()) {
            if (kDown & HidNpadButton_Down) {
                selected_idx = (selected_idx + 1) % (int)games.size();
            }
            if (kDown & HidNpadButton_Up) {
                selected_idx = (selected_idx - 1 + (int)games.size()) % (int)games.size();
            }
            if (kDown & HidNpadButton_A) {
                trigger_conversion = true;
            }

            if (touch_tap && touch_y >= 160 && touch_y <= 640) {
                int max_visible = 6;
                int start_idx = 0;
                if (selected_idx >= max_visible) {
                    start_idx = selected_idx - max_visible + 1;
                }

                for (int i = start_idx; i < (int)games.size() && i < start_idx + max_visible; ++i) {
                    int card_y = 160 + (i - start_idx) * 80;
                    if (touch_y >= card_y && touch_y <= card_y + 70 && touch_x >= 40 && touch_x <= 1240) {
                        if (selected_idx == i) {
                            trigger_conversion = true;
                        } else {
                            selected_idx = i;
                        }
                        break;
                    }
                }
            }
        }

        RenderFullBackground();

        if (active_tab_idx == 0 && !games.empty() && trigger_conversion) {
            std::string in_path = games[selected_idx].path;
            std::string out_path = in_path.substr(0, in_path.find_last_of('.')) + ".zcci";

            bool proceed_conversion = true;
            struct stat st_exist;
            if (stat(out_path.c_str(), &st_exist) == 0) {
                bool overwrite_modal = true;
                bool prev_ow_touch = false;

                while (overwrite_modal && appletMainLoop()) {
                    padUpdate(&pad);
                    u64 kOW = padGetButtonsDown(&pad);

                    HidTouchScreenState ow_st;
                    hidGetTouchScreenStates(&ow_st, 1);
                    bool ow_down = (ow_st.count > 0);
                    bool ow_tap = (ow_down && !prev_ow_touch);
                    prev_ow_touch = ow_down;

                    int tx = ow_down ? ow_st.touches[0].x : 0;
                    int ty = ow_down ? ow_st.touches[0].y : 0;

                    if ((kOW & (HidNpadButton_X | HidNpadButton_Y)) ||
                        (ow_tap && tx >= 180 && tx <= 600 && ty >= 415 && ty <= 485)) {
                        proceed_conversion = true;
                        overwrite_modal = false;
                    } else if ((kOW & (HidNpadButton_A | HidNpadButton_B)) ||
                               (ow_tap && tx >= 640 && tx <= 1060 && ty >= 415 && ty <= 485)) {
                        proceed_conversion = false;
                        overwrite_modal = false;
                    }

                    RenderFullBackground();

                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
                    SDL_Rect dark_tint = { 0, 0, 1280, 720 };
                    SDL_RenderFillRect(renderer, &dark_tint);

                    SDL_Rect modal_r = { 140, 140, 1000, 440 };
                    SDL_SetRenderDrawColor(renderer, c_header_bg.r, c_header_bg.g, c_header_bg.b, 255);
                    SDL_RenderFillRect(renderer, &modal_r);

                    SDL_Rect top_accent = { 140, 140, 1000, 10 };
                    SDL_SetRenderDrawColor(renderer, 252, 211, 77, 255);
                    SDL_RenderFillRect(renderer, &top_accent);

                    DrawString(renderer, "ARCHIVO .ZCCI EXISTENTE", 180, 160, c_text_yellow, 3);
                    DrawString(renderer, "Ya existe un archivo comprimido .zcci para este juego:", 180, 210, c_text_white, 2);
                    DrawString(renderer, out_path.c_str(), 180, 245, c_text_yellow, 2);

                    DrawString(renderer, "\x0A""Deseas sobrescribir el archivo .zcci existente?", 180, 310, c_text_white, 2);

                    bool hover_b1 = (tx >= 180 && tx <= 600 && ty >= 415 && ty <= 485);
                    bool hover_b2 = (tx >= 640 && tx <= 1060 && ty >= 415 && ty <= 485);

                    SDL_Rect btn1_r = { 180, 420, 420, 60 };
                    SDL_SetRenderDrawColor(renderer, (hover_b1 ? 239 : 220), (hover_b1 ? 68 : 38), (hover_b1 ? 68 : 38), 255);
                    SDL_RenderFillRect(renderer, &btn1_r);
                    DrawString(renderer, "[X / Y] Sobrescribir", 200, 438, c_text_white, 2);

                    SDL_Rect btn2_r = { 640, 420, 420, 60 };
                    SDL_SetRenderDrawColor(renderer, (hover_b2 ? 52 : c_accent_green.r), (hover_b2 ? 211 : c_accent_green.g), (hover_b2 ? 153 : c_accent_green.b), 255);
                    SDL_RenderFillRect(renderer, &btn2_r);
                    DrawString(renderer, "[A / B] Cancelar", 660, 438, c_text_black, 2);

                    SDL_RenderPresent(renderer);
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                }
            }

            if (proceed_conversion) {
                ProgressState st{};
                st.is_running = true;

                std::thread worker([&in_path, &out_path, &st]() {
                    st.result = Compress3DSToZCCI(
                        in_path, out_path,
                        [&st](uint64_t cur, uint64_t tot) {
                            st.current_bytes = cur;
                            st.total_bytes = tot;
                        }
                    );
                    st.is_running = false;
                });

                auto start_time = std::chrono::steady_clock::now();

                while (st.is_running && appletMainLoop()) {
                    uint64_t cur = st.current_bytes.load();
                    uint64_t tot = st.total_bytes.load();

                    auto now_time = std::chrono::steady_clock::now();
                    double elapsed_sec = std::chrono::duration<double>(now_time - start_time).count();

                    double speed_mbps = 0.0;
                    double eta_sec = 0.0;
                    if (elapsed_sec > 0.5 && cur > 0) {
                        double bytes_per_sec = (double)cur / elapsed_sec;
                        speed_mbps = bytes_per_sec / (1024.0 * 1024.0);
                        if (tot > cur && bytes_per_sec > 0) {
                            eta_sec = (double)(tot - cur) / bytes_per_sec;
                        }
                    }

                    RenderFullBackground();

                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
                    SDL_Rect dark_tint = { 0, 0, 1280, 720 };
                    SDL_RenderFillRect(renderer, &dark_tint);

                    SDL_Rect modal_r = { 140, 140, 1000, 440 };
                    SDL_SetRenderDrawColor(renderer, c_header_bg.r, c_header_bg.g, c_header_bg.b, 255);
                    SDL_RenderFillRect(renderer, &modal_r);

                    DrawString(renderer, "PROCESANDO COMPRESI\x05N ZCCI", 180, 170, c_text_yellow, 3);
                    DrawString(renderer, games[selected_idx].filename.c_str(), 180, 220, c_text_white, 2);

                    DrawString(renderer, "La compresi\x05n tardar\x01 proporcionalmente al tama\x07o.", 180, 260, c_text_gray, 2);
                    DrawString(renderer, "Por favor no apagues la consola.", 180, 290, c_text_gray, 2);

                    SDL_Rect pbar_bg = { 180, 340, 920, 40 };
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                    SDL_RenderFillRect(renderer, &pbar_bg);

                    float pct = (tot > 0) ? (float)cur / (float)tot : 0.0f;
                    SDL_Rect pbar_fg = { 180, 340, (int)(920 * pct), 40 };
                    SDL_SetRenderDrawColor(renderer, c_accent_green.r, c_accent_green.g, c_accent_green.b, 255);
                    SDL_RenderFillRect(renderer, &pbar_fg);

                    char stat_buf[128];
                    snprintf(stat_buf, sizeof(stat_buf), "%d%%  (%s / %s)", (int)(pct * 100.0f), FormatSize(cur).c_str(), FormatSize(tot).c_str());
                    DrawString(renderer, stat_buf, 180, 400, c_text_white, 2);

                    snprintf(stat_buf, sizeof(stat_buf), "%.2f MB/s   |   ETA: %s", speed_mbps, FormatETA(eta_sec).c_str());
                    DrawString(renderer, stat_buf, 180, 440, c_text_yellow, 2);

                    SDL_RenderPresent(renderer);
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                }

                if (worker.joinable()) {
                    worker.join();
                }

                if (st.result.load() == CompressResult::Success) {
                    bool prompt_modal = true;
                    bool prev_modal_touch = false;
                    while (prompt_modal && appletMainLoop()) {
                        padUpdate(&pad);
                        u64 kModal = padGetButtonsDown(&pad);

                        HidTouchScreenState t_st;
                        hidGetTouchScreenStates(&t_st, 1);
                        bool t_down = (t_st.count > 0);
                        bool t_tap = (t_down && !prev_modal_touch);
                        prev_modal_touch = t_down;

                        int tx = t_down ? t_st.touches[0].x : 0;
                        int ty = t_down ? t_st.touches[0].y : 0;

                        if ((kModal & (HidNpadButton_X | HidNpadButton_Y)) ||
                            (t_tap && tx >= 180 && tx <= 600 && ty >= 415 && ty <= 485)) {
                            remove(in_path.c_str());
                            prompt_modal = false;
                        }
                        else if ((kModal & (HidNpadButton_A | HidNpadButton_B)) ||
                                 (t_tap && tx >= 640 && tx <= 1060 && ty >= 415 && ty <= 485)) {
                            prompt_modal = false;
                        }

                        RenderFullBackground();

                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
                        SDL_Rect dark_tint = { 0, 0, 1280, 720 };
                        SDL_RenderFillRect(renderer, &dark_tint);

                        SDL_Rect modal_r = { 140, 140, 1000, 440 };
                        SDL_SetRenderDrawColor(renderer, c_header_bg.r, c_header_bg.g, c_header_bg.b, 255);
                        SDL_RenderFillRect(renderer, &modal_r);

                        SDL_Rect top_accent = { 140, 140, 1000, 10 };
                        SDL_SetRenderDrawColor(renderer, c_accent_green.r, c_accent_green.g, c_accent_green.b, 255);
                        SDL_RenderFillRect(renderer, &top_accent);

                        DrawString(renderer, "COMPRESI\x05N COMPLETADA EXITOSAMENTE!", 180, 160, c_accent_green, 3);
                        DrawString(renderer, "Juego original:", 180, 205, c_text_gray, 2);
                        DrawString(renderer, games[selected_idx].filename.c_str(), 180, 230, c_text_white, 2);

                        size_t last_slash = out_path.find_last_of('/');
                        std::string folder_path = (last_slash != std::string::npos) ? out_path.substr(0, last_slash + 1) : "sdmc:/";
                        std::string out_filename = (last_slash != std::string::npos) ? out_path.substr(last_slash + 1) : out_path;

                        std::string gen_label = "Archivo .zcci generado: " + folder_path;
                        DrawString(renderer, gen_label.c_str(), 180, 265, c_text_yellow, 2);
                        DrawString(renderer, out_filename.c_str(), 180, 290, c_text_white, 2);

                        DrawString(renderer, "\x0A""Deseas eliminar la ROM original .3ds para liberar espacio", 180, 335, c_text_white, 2);
                        DrawString(renderer, "en la tarjeta SD?", 180, 365, c_text_white, 2);

                        bool hover_b1 = (tx >= 180 && tx <= 600 && ty >= 415 && ty <= 485);
                        bool hover_b2 = (tx >= 640 && tx <= 1060 && ty >= 415 && ty <= 485);

                        SDL_Rect btn1_r = { 180, 420, 420, 60 };
                        SDL_SetRenderDrawColor(renderer, (hover_b1 ? 239 : 220), (hover_b1 ? 68 : 38), (hover_b1 ? 68 : 38), 255);
                        SDL_RenderFillRect(renderer, &btn1_r);
                        DrawString(renderer, "[X / Y] Eliminar .3ds", 200, 438, c_text_white, 2);

                        SDL_Rect btn2_r = { 640, 420, 420, 60 };
                        SDL_SetRenderDrawColor(renderer, (hover_b2 ? 52 : c_accent_green.r), (hover_b2 ? 211 : c_accent_green.g), (hover_b2 ? 153 : c_accent_green.b), 255);
                        SDL_RenderFillRect(renderer, &btn2_r);
                        DrawString(renderer, "[A / B] Conservar ambos", 660, 438, c_text_black, 2);

                        SDL_RenderPresent(renderer);
                        std::this_thread::sleep_for(std::chrono::milliseconds(16));
                    }
                } else {
                    bool prompt_modal = true;
                    bool prev_modal_touch = false;
                    while (prompt_modal && appletMainLoop()) {
                        padUpdate(&pad);
                        u64 kModal = padGetButtonsDown(&pad);

                        HidTouchScreenState t_st;
                        hidGetTouchScreenStates(&t_st, 1);
                        bool t_down = (t_st.count > 0);
                        bool t_tap = (t_down && !prev_modal_touch);
                        prev_modal_touch = t_down;

                        if ((kModal & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Y)) || t_tap) {
                            prompt_modal = false;
                        }

                        RenderFullBackground();

                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
                        SDL_Rect dark_tint = { 0, 0, 1280, 720 };
                        SDL_RenderFillRect(renderer, &dark_tint);

                        SDL_Rect modal_r = { 140, 140, 1000, 440 };
                        SDL_SetRenderDrawColor(renderer, c_header_bg.r, c_header_bg.g, c_header_bg.b, 255);
                        SDL_RenderFillRect(renderer, &modal_r);

                        DrawString(renderer, "ERROR EN LA COMPRESI\x05N", 180, 175, c_text_yellow, 3);
                        DrawString(renderer, "No se pudo completar la conversi\x05n del archivo.", 180, 240, c_text_white, 2);
                        DrawString(renderer, "Presiona cualquier bot\x05n o toca la pantalla para regresar.", 180, 380, c_text_gray, 2);

                        SDL_RenderPresent(renderer);
                        std::this_thread::sleep_for(std::chrono::milliseconds(16));
                    }
                }
            }

            games.clear();
            for (const auto& p : paths) {
                auto found = ScanDirectory(p.c_str());
                games.insert(games.end(), found.begin(), found.end());
            }
            if (selected_idx >= (int)games.size()) {
                selected_idx = std::max(0, (int)games.size() - 1);
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

} // namespace SwitchZCCI
