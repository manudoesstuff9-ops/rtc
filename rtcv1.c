// 'c': cycle color of selected circle
// 'r': toggle ray debug lines

#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINW 1400
#define WINH 800
#define TWO_PI (6.28318530717958647692)
#define EPS2D (1e-4)

typedef struct { double x, y; } Vec2;
static inline Vec2 v2(double x, double y){ Vec2 a={x,y}; return a; }
static inline Vec2 add2(Vec2 a, Vec2 b){ return v2(a.x+b.x, a.y+b.y); }
static inline Vec2 sub2(Vec2 a, Vec2 b){ return v2(a.x-b.x, a.y-b.y); }
static inline Vec2 muls2(Vec2 a, double s){ return v2(a.x*s, a.y*s); }
static inline double dot2(Vec2 a, Vec2 b){ return a.x*b.x + a.y*b.y; }
static inline double len2(Vec2 a){ return sqrt(dot2(a,a)); }
static inline Vec2 norm2(Vec2 a){ double L=len2(a); if(L==0) return a; return muls2(a,1.0/L); }


int ray_circle_intersect(Vec2 o, Vec2 d, Vec2 c, double r, double *t_near, double *t_far){
    Vec2 oc = sub2(o, c);
    double a = dot2(d,d);                          
    double b = 2.0 * dot2(oc, d);
    double cc = dot2(oc, oc) - r*r;
    double disc = b*b - 4*a*cc;
    if (disc < 0.0) return 0;
    double sq = sqrt(disc);
    double t0 = (-b - sq) / (2*a);
    double t1 = (-b + sq) / (2*a);
    if (t0 > t1){ double tmp = t0; t0 = t1; t1 = tmp; }
    *t_near = t0; *t_far = t1;
    return 1;
}

void draw_thick_line(SDL_Renderer *rend, int x1,int y1,int x2,int y2, int thickness){
    if (thickness <= 1){ SDL_RenderDrawLine(rend, x1,y1,x2,y2); return; }
    double dx = (double)(x2 - x1);
    double dy = (double)(y2 - y1);
    double L = sqrt(dx*dx + dy*dy);
    if (L < 1e-6){ SDL_RenderDrawPoint(rend, x1, y1); return; }
    /* perpendicular unit vector (normalized) */
    double px = -dy / L;
    double py = dx / L;
    int half = thickness/2;
    for (int t = -half; t <= half; t++){
        int ox = (int)round(px * (double)t);
        int oy = (int)round(py * (double)t);
        SDL_RenderDrawLine(rend, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
    }
}

typedef struct {
    Vec2 c;
    double r;
    Uint8 color[3]; 
} Circle;

int main(int argc, char **argv){
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("2D Shadow Demo",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       WINW, WINH, SDL_WINDOW_SHOWN);
    if (!win){ fprintf(stderr,"CreateWindow: %s\n", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!rend){ fprintf(stderr,"CreateRenderer: %s\n", SDL_GetError()); SDL_DestroyWindow(win); SDL_Quit(); return 1; }

    const int MAXC = 8;
    Circle circles[MAXC];
    int ncircles = 3;
    circles[0].c = v2(WINW*0.35, WINH*0.5); circles[0].r = 70.0; circles[0].color[0]=200; circles[0].color[1]=40; circles[0].color[2]=40;
    circles[1].c = v2(WINW*0.6,  WINH*0.45); circles[1].r = 60.0; circles[1].color[0]=40;  circles[1].color[1]=200; circles[1].color[2]=80;
    circles[2].c = v2(WINW*0.65, WINH*0.7);  circles[2].r = 40.0; circles[2].color[0]=40;  circles[2].color[1]=120; circles[2].color[2]=200;

    Vec2 light = v2(WINW*0.2, WINH*0.2);

    int running = 1;
    SDL_Event e;
    int show_rays = 0;
    int samples = 720; 

    int dragging = 0;           
    int selected = -1;         
    int mouse_down = 0;

    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

    while (running){
        while (SDL_PollEvent(&e)){
            if (e.type == SDL_QUIT) { running = 0; break; }

            else if (e.type == SDL_MOUSEMOTION){
                if (!dragging){
                    light.x = e.motion.x;
                    light.y = e.motion.y;
                } else if (selected >= 0 && selected < ncircles){
                    circles[selected].c.x = e.motion.x;
                    circles[selected].c.y = e.motion.y;
                }
            }

            else if (e.type == SDL_MOUSEBUTTONDOWN){
                mouse_down = 1;
                if (e.button.button == SDL_BUTTON_LEFT){
                    int found = -1;
                    double best_dist = 1e20;
                    for (int i = 0; i < ncircles; ++i){
                        double d = len2(sub2(v2(e.button.x, e.button.y), circles[i].c));
                        if (d <= circles[i].r && d < best_dist){
                            best_dist = d;
                            found = i;
                        }
                    }
                    if (found >= 0){
                        selected = found;
                        dragging = 1;
                    } else {
                        light.x = e.button.x;
                        light.y = e.button.y;
                        selected = -1;
                        dragging = 0;
                    }
                } else if (e.button.button == SDL_BUTTON_RIGHT){
                    show_rays = !show_rays;
                }
            }

            else if (e.type == SDL_MOUSEBUTTONUP){
                mouse_down = 0;
                if (e.button.button == SDL_BUTTON_LEFT){
                    dragging = 0;
                }
            }

            else if (e.type == SDL_MOUSEWHEEL){
                if (selected >= 0 && selected < ncircles){
                    circles[selected].r += 6.0 * e.wheel.y;
                    if (circles[selected].r < 4.0) circles[selected].r = 4.0;
                } else {
                    if (e.wheel.y > 0) samples += 120;
                    else if (e.wheel.y < 0) samples = (samples-120>60) ? samples-120 : 60;
                }
            }

            else if (e.type == SDL_KEYDOWN){
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_ESCAPE) { running = 0; break; }
                else if (k == SDLK_r) show_rays = !show_rays;
                else if (k == SDLK_UP) samples += 120;
                else if (k == SDLK_DOWN) samples = (samples-120>60) ? samples-120 : 60;
                else if (k == SDLK_c){
                    if (selected >= 0 && selected < ncircles){
                        Uint8 r = circles[selected].color[0];
                        Uint8 g = circles[selected].color[1];
                        Uint8 b = circles[selected].color[2];
                        circles[selected].color[0] = g;
                        circles[selected].color[1] = b;
                        circles[selected].color[2] = r;
                    }
                } else if (k == SDLK_PLUS || k == SDLK_EQUALS){
                    if (selected >= 0 && selected < ncircles) circles[selected].r += 4.0;
                } else if (k == SDLK_MINUS){
                    if (selected >= 0 && selected < ncircles){
                        circles[selected].r -= 4.0;
                        if (circles[selected].r < 4.0) circles[selected].r = 4.0;
                    }
                } else if (k == SDLK_a){ 
                    if (ncircles < MAXC){
                        circles[ncircles].c = light;
                        circles[ncircles].r = 40.0;
                        circles[ncircles].color[0]=180; circles[ncircles].color[1]=80; circles[ncircles].color[2]=200;
                        selected = ncircles;
                        ncircles++;
                    }
                } else if (k == SDLK_DELETE || k == SDLK_BACKSPACE){
                    if (selected >= 0 && selected < ncircles){
                        for (int i = selected; i+1 < ncircles; ++i) circles[i] = circles[i+1];
                        ncircles--;
                        selected = -1;
                        dragging = 0;
                    }
                }
            }
        } /* end event loop */

        SDL_SetRenderDrawColor(rend, 200, 220, 255, 255);
        SDL_RenderClear(rend);

        for (int gx = 0; gx < WINW; gx += 40){
            SDL_SetRenderDrawColor(rend, 220,230,240,255);
            SDL_RenderDrawLine(rend, gx, 0, gx, WINH);
        }
        for (int gy = 0; gy < WINH; gy += 40){
            SDL_SetRenderDrawColor(rend, 220,230,240,255);
            SDL_RenderDrawLine(rend, 0, gy, WINW, gy);
        }

        SDL_SetRenderDrawColor(rend, 0, 0, 0, 180); 

        for (int s = 0; s < samples; s++){
            double a = (double)s / (double)samples * TWO_PI;
            Vec2 dir = v2(cos(a), sin(a));  

            double best_tfar = -1.0;
            int hit_idx = -1;

            for (int ci = 0; ci < ncircles; ++ci){
                double tnear, tfar;
                if (ray_circle_intersect(light, dir, circles[ci].c, circles[ci].r, &tnear, &tfar)){
                    if (tfar > EPS2D){
                        if (best_tfar < 0 || tfar < best_tfar){
                            best_tfar = tfar;
                            hit_idx = ci;
                        }
                    }
                }
            }

            if (hit_idx >= 0){
                if (!(best_tfar > EPS2D)) continue;

                Vec2 hitp = add2(light, muls2(dir, best_tfar));
                if (len2(sub2(hitp, light)) < 1.5) continue; 

                Vec2 farpt = add2(hitp, muls2(dir, 2000.0));
                int x1 = (int)round(hitp.x), y1 = (int)round(hitp.y);
                int x2 = (int)round(farpt.x), y2 = (int)round(farpt.y);

                draw_thick_line(rend, x1,y1,x2,y2, 8);

                if (show_rays){
                    SDL_SetRenderDrawColor(rend, 255, 180, 0, 120);
                    SDL_RenderDrawLine(rend, (int)light.x, (int)light.y, x1, y1);
                    SDL_SetRenderDrawColor(rend, 0,0,0,180);
                }
            } else {
                if (show_rays){
                    Vec2 farpt = add2(light, muls2(dir, 1200.0));
                    SDL_SetRenderDrawColor(rend, 255, 180, 0, 30);
                    SDL_RenderDrawLine(rend, (int)light.x, (int)light.y, (int)farpt.x, (int)farpt.y);
                    SDL_SetRenderDrawColor(rend, 0,0,0,180);
                }
            }
        }

        for (int ci = 0; ci < ncircles; ++ci){
            SDL_SetRenderDrawColor(rend, circles[ci].color[0], circles[ci].color[1], circles[ci].color[2], 255);
            double circle_r = circles[ci].r;
            Vec2 circle_c = circles[ci].c;
            for (double rr = 0; rr <= circle_r; rr += 1.0){
                int prevx = (int)(circle_c.x + rr * cos(0));
                int prevy = (int)(circle_c.y + rr * sin(0));
                for (double aa = 0; aa <= TWO_PI + 0.01; aa += 0.02){
                    int x = (int)round(circle_c.x + rr * cos(aa));
                    int y = (int)round(circle_c.y + rr * sin(aa));
                    SDL_RenderDrawLine(rend, prevx, prevy, x, y);
                    prevx = x; prevy = y;
                }
            }

            if (ci == selected){
                SDL_SetRenderDrawColor(rend, 255,255,255,200);
                int prevx = (int)(circle_c.x + circle_r * cos(0));
                int prevy = (int)(circle_c.y + circle_r * sin(0));
                for (double aa = 0; aa <= TWO_PI + 0.01; aa += 0.02){
                    int x = (int)round(circle_c.x + circle_r * cos(aa));
                    int y = (int)round(circle_c.y + circle_r * sin(aa));
                    SDL_RenderDrawLine(rend, prevx, prevy, x, y);
                    prevx = x; prevy = y;
                }
            }
        }

        SDL_SetRenderDrawColor(rend, 255, 230, 80, 255);
        for (int r = 0; r <= 6; r++){
            int prevx = (int)(light.x + r * cos(0));
            int prevy = (int)(light.y + r * sin(0));
            for (double aa = 0; aa <= TWO_PI + 0.01; aa += 0.1){
                int x = (int)round(light.x + r * cos(aa));
                int y = (int)round(light.y + r * sin(aa));
                SDL_RenderDrawLine(rend, prevx, prevy, x, y);
                prevx = x; prevy = y;
            }
        }

        char title[256];
        snprintf(title, sizeof(title),
                 "2D Shadow Demo — rays=%d (Up/Down)  R toggles rays  LClick-drag circles  Wheel/+/ - resize  C cycle color  A add  Del remove",
                 samples);
        SDL_SetWindowTitle(win, title);

        SDL_RenderPresent(rend);
        SDL_Delay(10);
    }

    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
