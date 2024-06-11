#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

const int FPS = 60.0;
struct { double x; double y; double vx; double vy; } ball;
int currentScore = 0;
int highScore = 0;
SDL_Surface* scoreFontSpriteSheet = NULL;

Uint64 prev, now; // timers
double delta_t;  // durée frame en ms
int x_vault;

SDL_Window* pWindow = NULL;
SDL_Surface* win_surf = NULL;
SDL_Surface* plancheSprites = NULL;
SDL_Surface* plancheBrick = NULL;

SDL_Rect srcBg = { 0, 128, 96, 128 }; // x,y, w,h (0,0) en haut a gauche
SDL_Rect srcBall = { 0, 96, 24, 24 };
SDL_Rect srcVaiss = { 128, 0, 128, 32 };

SDL_Rect srcBrique = { 0, 0, 30, 16 };  // Définir la taille des briques

int current_level = 1; // Ajouter cette variable globale pour suivre le niveau actuel

const char* filename = "niveau.txt";

typedef struct {
    int x, y;
    bool visible;
} Brique;

#define NB_BRIQUES_LIGNE 20
#define NB_BRIQUES_COLONNE 16
Brique briques[NB_BRIQUES_LIGNE * NB_BRIQUES_COLONNE];

typedef struct {
    int x, y;
    bool visible;
    int type;  // different types of bonuses
} Bonus;

#define NB_BONUS 10
Bonus bonus[NB_BONUS];

#define MAX_BALLS 5
struct { double x; double y; double vx; double vy; } balls[MAX_BALLS];

const int FONT_WIDTH = 16;
const int FONT_HEIGHT = 24;

const SDL_Rect srcRects[128] = {
    [0 ... 127] = {0, 0, 0, 0},

    ['0' - 33] = {0 * FONT_WIDTH, 1.5 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['1' - 33] = {2 * FONT_WIDTH, 1.5 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['2' - 33] = {4 * FONT_WIDTH, 1.5 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['3' - 33] = {6 * FONT_WIDTH, 1.5 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['4' - 33] = {8 * FONT_WIDTH, 1.5 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['5' - 33] = {10 * FONT_WIDTH, 1.5 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['6' - 33] = {12 * FONT_WIDTH, 1.5 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['7' - 33] = {14 * FONT_WIDTH, 1.5 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['8' - 33] = {16 * FONT_WIDTH, 1.5 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['9' - 33] = {18 * FONT_WIDTH, 1.5 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},

    ['S' - 33] = {6 * FONT_WIDTH, 4.25 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['C' - 33] = {6 * FONT_WIDTH, 2.95 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['O' - 33] = {30 * FONT_WIDTH, 2.95 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['R' - 33] = {4 * FONT_WIDTH, 4.25 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['E' - 33] = {10 * FONT_WIDTH, 2.95 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['H' - 33] = {16 * FONT_WIDTH, 2.95 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['I' - 33] = {18 * FONT_WIDTH, 2.95 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['G' - 33] = {14 * FONT_WIDTH, 2.95 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    ['H' - 33] = {16 * FONT_WIDTH, 2.95 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
    [':' - 33] = {20 * FONT_WIDTH, 1.65 * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT},
};

void initializeScore() {
    scoreFontSpriteSheet = SDL_LoadBMP("./Arkanoid_ascii.bmp");
    if (scoreFontSpriteSheet == NULL) {
        printf("Unable to load score font sprite sheet: %s\n", SDL_GetError());
        exit(1); // Exit if the sprite sheet failed to load
    }
    SDL_SetColorKey(scoreFontSpriteSheet, SDL_TRUE, SDL_MapRGB(scoreFontSpriteSheet->format, 0, 0, 0));
    currentScore = 0;
    highScore = 0;

    FILE* file = fopen("highscore.txt", "r"); // Open the high score file for reading
    if (file != NULL) { // Ensure the file was opened successfully
        if (fscanf(file, "%d", &highScore) != 1) {
            highScore = 0; // If reading fails, default the highScore to 0
        }
        fclose(file); // Close the file
    } else {
        highScore = 0; // If the file doesn't exist, default the highScore to 0
    }
}

void drawText(SDL_Surface *surface, const char *text, int x, int y) {
    int len = strlen(text);

    for (int i = 0; i < len; ++i) {
        char character = text[i];
        if (character - 33 < 0 || character - 33 >= 128) continue;

        SDL_Rect srcRect = srcRects[character - 33];
        SDL_Rect dstRect = {x + (i * FONT_WIDTH), y, FONT_WIDTH, FONT_HEIGHT};
        SDL_BlitSurface(scoreFontSpriteSheet, &srcRect, surface, &dstRect);
    }
}

void drawScore(SDL_Surface *surface, int score, int x, int y) {
    char scoreStr[12];
    sprintf(scoreStr, "%d", score);
    int scoreLength = strlen(scoreStr);

    int scoreWidth = scoreLength * FONT_WIDTH;

    x -= scoreWidth;

    SDL_Rect clearRect = {x, y, scoreWidth, FONT_HEIGHT};
    SDL_FillRect(surface, &clearRect, SDL_MapRGB(surface->format, 0, 0, 0));

    for (int i = 0; scoreStr[i] != '\0'; ++i) {
        char character = scoreStr[i];
        SDL_Rect srcRect = srcRects[character - '!'];
        SDL_Rect dstRect = {x + (i * FONT_WIDTH), y, FONT_WIDTH, FONT_HEIGHT};
        SDL_BlitSurface(scoreFontSpriteSheet, &srcRect, surface, &dstRect);
    }
}

void init_briques() {
    // Modifier cette ligne pour utiliser current_level dans le nom du fichier
    char filename[20];
    sprintf(filename, "niveau%d.txt", current_level);

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erreur de chargement du fichier : %s\n", filename);
        exit(1);
    }

    int index = 0;
    for (int i = 0; i < NB_BRIQUES_COLONNE; i++) {
        for (int j = 0; j < NB_BRIQUES_LIGNE; j++) {
            int visible;
            fscanf(file, "%d", &visible);
            briques[index].x = j * (srcBrique.w + 1) - 1;  // largeur brique + espace
            briques[index].y = i * (srcBrique.h + 1) - 1 + 110;  // hauteur brique + espace + décalage
            briques[index].visible = visible;
            index++;
        }
    }

    // Initialize bonus list
    for (int i = 0; i < NB_BONUS; i++) {
        bonus[i].x = -10;  // out of the screen
        bonus[i].y = -10;
        bonus[i].visible = false;
        bonus[i].type = 0;
    }

    // Initialize additional balls
    for (int i = 0; i < MAX_BALLS; i++) {
        balls[i].x = 0;
        balls[i].y = 0;
        balls[i].vx = 0;
        balls[i].vy = 0;
    }

    fclose(file);
}

void draw_bonus(SDL_Surface *surface, Bonus *b) {
    if (!b->visible) return;

    int bonus_x = (b->type - 1) / 7 * 33 + 256;  // adjust the x position based on the bonus type
    int bonus_y = (b->type - 1) % 7 * 16;  // adjust the y position based on the bonus type
    SDL_Rect src = {bonus_x, bonus_y, 32, 14};
    SDL_Rect dst = {b->x, b->y, 32, 14};
    SDL_BlitSurface(plancheBrick, &src, surface, &dst);
}

void update_bonus(Bonus *b) {
    if (!b->visible) return;

    b->y += 1;  // move down

    // check collision with the paddle
    if (b->y + 32 >= win_surf->h - 32 && b->y + 32 <= win_surf->h - 20 && b->x + 16 >= x_vault && b->x + 16 <= x_vault + srcVaiss.w) {
        b->visible = false;
                    // apply the bonus effect
        switch (b->type) {
            case 1:  // increase paddle size
                srcVaiss.w += 32;
                break;
            case 2:  // extra life
                break;
            case 3:  // ball goes through bricks
                srcBall.y = 128;  // change to the blue ball sprite
                break;
            case 4:  // slow down ball
                ball.vx *= 0.5;
                ball.vy *= 0.5;
                break;
            case 5:  // speed up ball
                ball.vx *= 2;
                ball.vy *= 2;
                break;
            case 6:  // multi-ball
                for (int i = 0; i < MAX_BALLS; i++) {
                    if (balls[i].x == 0 && balls[i].y == 0) {
                        balls[i].x = ball.x;
                        balls[i].y = ball.y;
                        balls[i].vx = ball.vx;
                        balls[i].vy = ball.vy;
                        // add a random variation to the velocity
                        balls[i].vx += (rand() % 100 + 1) * 0.01 * (rand() % 2 ? 1 : -1);
                        balls[i].vy += (rand() % 100 + 1) * 0.01 * (rand() % 2 ? 1 : -1);
                        break;
                    }
                }
                break;
            case 7:  // laser
                break;
        }
    }

    // check if the bonus is out of the screen
    if (b->y > win_surf->h) {
        b->visible = false;
    }
}

void draw_additional_balls(SDL_Surface *surface) {
    SDL_Rect dstBall = { 0, 0, 24, 24 };
    for (int i = 0; i < MAX_BALLS; i++) {
        if (balls[i].x != 0 && balls[i].y != 0) {
            dstBall.x = (int)balls[i].x;
            dstBall.y = (int)balls[i].y;
            SDL_BlitSurface(plancheSprites, &srcBall, surface, &dstBall);
        }
    }
}

void update_additional_balls() {
    for (int i = 0; i < MAX_BALLS; i++) {
        if (balls[i].x != 0 && balls[i].y != 0) {
            // Déplacement de la balle
            balls[i].x += balls[i].vx;
            balls[i].y += balls[i].vy;

            // Collision avec les bords de la fenêtre
            if ((balls[i].x < 0) || (balls[i].x > (win_surf->w - 24)))
                balls[i].vx *= -1;
            if (balls[i].y < 0)
                balls[i].vy *= -1;

            // Collision avec la barre de score en haut
            if (balls[i].y <= 50 && balls[i].x + 24 >= 0 && balls[i].x <= win_surf->w - 24) {
                balls[i].vy *= -1;
            }

            // Collision balle-briques
            for (int j = 0; j < NB_BRIQUES_LIGNE * NB_BRIQUES_COLONNE; j++) {
                if (briques[j].visible) {
                    if (balls[i].x + 24 > briques[j].x && balls[i].x < briques[j].x + srcBrique.w &&
                        balls[i].y + 24 > briques[j].y && balls[i].y < briques[j].y + srcBrique.h) {
                        briques[j].visible = false;
                        balls[i].vy *= -1;
                        currentScore += 10;

                        // create a random bonus
                        for (int k = 0; k < NB_BONUS; k++) {
                            if (!bonus[k].visible) {
                                bonus[k].x = briques[j].x + 16;  // adjust the x position to center the bonus
                                bonus[k].y = briques[j].y + 16;
                                bonus[k].visible = true;
                                bonus[k].type = rand() % 14 + 1;  // 14 different types of bonuses
                                break;
                            }
                        }

                        break;  // pour éviter plusieurs collisions dans un seul frame
                    }
                }
            }

            // Collision balle-barre
            if (balls[i].y + 24 >= win_surf->h - 32 && balls[i].y + 24 <= win_surf->h - 20 && balls[i].x + 12 >= x_vault && balls[i].x + 12 <= x_vault + srcVaiss.w) {
                balls[i].vy *= -1;
                balls[i].y = win_surf->h - 32 - 24;  // repositionne la balle juste au-dessus de la barre
            }
        }
    }
}

void init() {
    srand(time(NULL));  // initialize the random number generator

    pWindow = SDL_CreateWindow("Arknoid", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 610, 600, SDL_WINDOW_SHOWN);
    win_surf = SDL_GetWindowSurface(pWindow);
    plancheSprites = SDL_LoadBMP("./sprites.bmp");
    if (plancheSprites == NULL) {
        printf("Erreur de chargement de l'image : %s\n", SDL_GetError());
        exit(1);
    }

    SDL_SetColorKey(plancheSprites, true, SDL_MapRGB(plancheSprites->format, 0, 0, 0));

    plancheBrick = SDL_LoadBMP("./Arkanoid_sprites.bmp");
    if (plancheBrick == NULL) {
        printf("Erreur de chargement de l'image : %s\n", SDL_GetError());
        exit(1);
    }

    initializeScore();

    // Centrer la balle sur la barre (vaisseau)
    ball.x = x_vault + (srcVaiss.w / 2) - (srcBall.w / 2);
    ball.y = win_surf->h - 32 - srcBall.h;

    ball.vx = 1.0;
    ball.vy = 1.4;

    init_briques();

    now = SDL_GetPerformanceCounter();
}

void draw() {
    // Remplir le fond avec le sprite de fond
    SDL_Rect dest = { 0, 0, 0, 0 };
    for (int j = 0; j < win_surf->h; j += 128)
        for (int i = 0; i < win_surf->w; i += 96) {
            dest.x = i;
            dest.y = j;
            SDL_BlitSurface(plancheBrick, &srcBg, win_surf, &dest);
        }

    // Afficher les briques
    SDL_Rect srcBrique = { 0, 0, 30, 16 };  // Définir la taille des briques
    for (int i = 0; i < NB_BRIQUES_LIGNE * NB_BRIQUES_COLONNE; i++) {
        if (briques[i].visible) {
            SDL_Rect dstBrique = { briques[i].x, briques[i].y, srcBrique.w, srcBrique.h };
            SDL_BlitSurface(plancheBrick, &srcBrique, win_surf, &dstBrique);
        }
    }

    // Afficher la balle
    SDL_Rect dstBall = { (int)ball.x, (int)ball.y, 24, 24 };
    SDL_BlitSurface(plancheSprites, &srcBall, win_surf, &dstBall);

    // Déplacement de la balle
    ball.x += ball.vx;
    ball.y += ball.vy;

    // Collision avec les bords de la fenêtre
    if ((ball.x < 0) || (ball.x > (win_surf->w - 24)))
        ball.vx *= -1;
    if (ball.y < 0)
        ball.vy *= -1;

    // Collision avec la barre de score en haut
    if (ball.y <= 50 && ball.x + 24 >= 0 && ball.x <= win_surf->w - 24) {
        ball.vy *= -1;
    }

    // Changer la couleur de la balle lorsqu'elle touche le bas
    if (ball.y > (win_surf->h - 24))
        srcBall.y = 64; // Balle rouge
    else
        srcBall.y = 96; // Balle verte

    // Afficher la barre (vaisseau)
    dest.x = x_vault;
    dest.y = win_surf->h - 32;
    SDL_BlitSurface(plancheSprites, &srcVaiss, win_surf, &dest);

    // Collision balle-vaisseau
    if (ball.y + 24 >= win_surf->h - 32 && ball.y + 24 <= win_surf->h - 20 && ball.x + 12 >= x_vault && ball.x + 12 <= x_vault + srcVaiss.w) {
        ball.vy *= -1;
        ball.y = win_surf->h - 32 - 24;  // repositionne la balle juste au-dessus de la barre

        // Augmenter la vitesse de la balle de 5% jusqu'à un maximum de 500%
        double speed_increase = 0.05;
        double current_speed_squared = ball.vx * ball.vx + ball.vy * ball.vy;
        double current_speed = 1.0;
        while (current_speed * current_speed < current_speed_squared) {
            current_speed *= 2.0;
        }
        while (current_speed * current_speed > current_speed_squared) {
            current_speed *= 0.5;
        }
        double new_speed = current_speed * (1 + speed_increase);
        if (new_speed <= 4.0) { // 2.0 est la vitesse maximale, vous pouvez l'ajuster selon vos besoins
            double scale_factor = new_speed / current_speed;
            ball.vx *= scale_factor;
            ball.vy *= scale_factor;
        }
    }

    // Collision balle-briques
    for (int i = 0; i < NB_BRIQUES_LIGNE * NB_BRIQUES_COLONNE; i++) {
        if (briques[i].visible) {
            if (ball.x + 24 > briques[i].x && ball.x < briques[i].x + srcBrique.w &&
                ball.y + 24 > briques[i].y && ball.y < briques[i].y + srcBrique.h) {
                briques[i].visible = false;
                ball.vy *= -1;
                currentScore += 10;

                // create a random bonus
                for (int j = 0; j < NB_BONUS; j++) {
                    if (!bonus[j].visible) {
                        bonus[j].x = briques[i].x + 16;  // adjust the x position to center the bonus
                        bonus[j].y = briques[i].y + 16;
                        bonus[j].visible = true;
                        bonus[j].type = rand() % 14 + 1;  // 14 different types of bonuses
                        break;
                    }
                }

                break;  // pour éviter plusieurs collisions dans un seul frame
            }
        }
    }

    // draw and update bonuses
    for (int i = 0; i < NB_BONUS; i++) {
        update_bonus(&bonus[i]);
        draw_bonus(win_surf, &bonus[i]);
    }

    // draw and update additional balls
    update_additional_balls();
    draw_additional_balls(win_surf);

    // Vérifier si toutes les briques sont détruites
    bool all_bricks_destroyed = true;
    for (int i = 0; i < NB_BRIQUES_LIGNE * NB_BRIQUES_COLONNE; i++) {
        if (briques[i].visible) {
            all_bricks_destroyed = false;
            break;
        }
    }

    // Passez au niveau suivant si toutes les briques sont détruites
    if (all_bricks_destroyed) {
        current_level++;
        init_briques();
        initializeScore();

        // Réinitialiser la balle
        ball.x = x_vault + (srcVaiss.w / 2) - (srcBall.w / 2);
        ball.y = win_surf->h - 32 - srcBall.h;
        ball.vx = 1.0;
        ball.vy = 1.4;

        // Réinitialiser les balles supplémentaires
        for (int i = 0; i < MAX_BALLS; i++) {
            balls[i].x = 0;
            balls[i].y = 0;
            balls[i].vx = 0;
            balls[i].vy = 0;
        }

        // Réinitialiser les bonus
        for (int i = 0; i < NB_BONUS; i++) {
            bonus[i].x = -10;
            bonus[i].y = -10;
            bonus[i].visible = false;
            bonus[i].type = 0;
        }

        // Réinitialiser la taille de la barre
        srcVaiss.w = 128;
    }

    // Afficher le score et le high score
    SDL_Rect scoreboardRect = {0, 0, win_surf->w, 50};
    SDL_FillRect(win_surf, &scoreboardRect, SDL_MapRGB(win_surf->format, 0, 0, 0));

    drawText(win_surf, "SCORE:", 10, 25);
    drawScore(win_surf, currentScore, 150, 25);
    drawText(win_surf, "HIGH SCORE:", win_surf->w - 350, 25);
    drawScore(win_surf, highScore, win_surf->w - 100, 25);
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        return 1;
    }

    init();

    bool quit = false;
    while (!quit) {
        SDL_PumpEvents();
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_LEFT])
            x_vault -= 10;
        if (keys[SDL_SCANCODE_RIGHT])
            x_vault += 10;
        if (keys[SDL_SCANCODE_ESCAPE])
            quit = true;

        // Empêcher la barre de dépasser les limites de la fenêtre
        if (x_vault < 0)
            x_vault = 0;
        if (x_vault > win_surf->w - srcVaiss.w)
            x_vault = win_surf->w - srcVaiss.w;

        draw();
        SDL_UpdateWindowSurface(pWindow);
        now = SDL_GetPerformanceCounter();
        delta_t = 1.0 / FPS - (double)(now - prev) / (double)SDL_GetPerformanceFrequency();
        prev = now;
        if (delta_t > 0)
            SDL_Delay((Uint32)(delta_t * 1000));
        printf("dt = %lf\n", delta_t * 1000);
        prev = SDL_GetPerformanceCounter();
    }

    SDL_Quit();
    return 0;
}
