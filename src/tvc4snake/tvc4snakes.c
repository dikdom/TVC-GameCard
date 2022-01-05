#include "tvc4snakes.h"
#include <stdio.h>
#include <tvc.h>
#include <time.h>
#include <games.h>
#include <stdlib.h>
#include <stdbool.h>
#include <intrinsic.h>

// #define EMU

#define RESET_TIME_COUNTER() intrinsic_di();*((int*)0x0b1d)=0;intrinsic_ei()
#define debug(x) tvc_ed_cpos(1,5) ; printf( x ) ; while(true){} 
#define debugns(x,y,z,a) tvc_ed_cpos(1,5) ; printf( x,y,z,a ) ; 

#ifdef EMU
#define DEBUG_SNAKECOLOR_BORDER(i) tvc_set_border(SNAKE_COLORS[ i ])
#define DEBUG_COLOR_BORDER(i) tvc_set_border( i )
#else
#define DEBUG_SNAKECOLOR_BORDER(i) ;
#define DEBUG_COLOR_BORDER(i) ;
#endif


#define MAX_SNAKE_SIZE 50
#define MAX_Y 115
#define MAX_X 128

#define BORDERCOLOR cyan

const u8_t SNAKE_COLORS[] = {lred, lgreen, lblue, yellow, purple};
const u8_t PREDEF_DIRECTIONS[] = {MOVE_LEFT, MOVE_UP, MOVE_RIGHT, MOVE_DOWN};
const char *ID="JOY+SN";

void draw_point(u8_t c, snake_point* point);
enum color get_screen_color(snake_point* point);

snake_point snake[MAX_PLAYERS+1][MAX_SNAKE_SIZE];     // used by AI
unsigned char snake_length[MAX_PLAYERS+1];            // used by AI
char snake_array_headpos[MAX_PLAYERS+1];              // used by AI
u8_t snake_directions[MAX_PLAYERS+1];               
bool snake_plays[MAX_PLAYERS+1];                      // used by AI
u8_t gamecard_slot = 4;   // not detected yet
i8_t gamecard_baseport = 0;
u8_t speed = 0;
bool food_on_screen = false;
u16_t score[MAX_PLAYERS];                             // not used by AI
u8_t food_count = 0;
snake_point food_point;
snake_point *ai_snake_head_point;
u8_t next_speed_change;
u8_t at_this_speed;

void init_score() {
  u16_t addr;
  u8_t c_byte;
  for(u8_t i = 0; i<4; i++) {
    score[i] = 0;
    addr = 0x8000 + 23*640 + i*16;
    c_byte = SNAKE_COLORS[i] | (SNAKE_COLORS[i]<<1);

    for(u8_t k = 0; k<10; k++) {
      for(u8_t j = 0; j<16; j++) {
        *((u8_t *)addr + j) = c_byte;
      }
      addr+=64;
    }
    if(!snake_plays[i])
      continue;
    tvc_ed_cpos((i*4)+2, 24);
    color_or_index coi;
    coi.color = SNAKE_COLORS[i];
    tvc_set_paper(coi);
    coi.color = black;
    tvc_set_ink(coi);
    printf("00");
  }
}

void print_score(u8_t player) {
  color_or_index coi;
  coi.color = SNAKE_COLORS[player];
  tvc_set_paper(coi);
  tvc_ed_cpos(player*4 + 2, 24);
  printf("%02u", score[player]);
}

void init_game() {
  snake[0][0].x = 3;
  snake[0][0].y = 3;
  snake[1][0].x = MAX_X - 4;
  snake[1][0].y = 3;
  snake[2][0].x = MAX_X - 4;
  snake[2][0].y = MAX_Y - 4;
  snake[3][0].x = 3;
  snake[3][0].y = MAX_Y - 4;
  snake_point *sp;
  for(unsigned char i = 0; i<MAX_PLAYERS+1; i++) {
    snake_length[i]=(char)1;
    snake_plays[i]=false;
    snake_array_headpos[i] = (char)0;
    sp = &snake[i][0];
    sp->addr = 0x8000 + ((unsigned short)sp->y << 7) + (sp->x/2);   // one snake pixel is half byte (4 bits) wide and 2 rows height
    score[i] = 0;
  }
  snake_directions[0] = MOVE_RIGHT;
  snake_directions[1] = MOVE_DOWN;
  snake_directions[2] = MOVE_LEFT;
  snake_directions[3] = MOVE_UP;
  speed = 1;
  food_on_screen = false;
  food_count = 0;
  food_point.addr = 0;
  next_speed_change = 3;
  at_this_speed = 3;
}

/**
 * Let's find the slot of the inserted 4 player Game Card
 */
void find_slot() {
  char *addr;
  for(unsigned char slot = 0; slot<4; slot++) {
    addr = (char *)(0x0040 + slot*0x0030);
    if(*addr != 6)
      continue;
    ++addr;
    char *idStr;
    idStr = ID;
    char c = 0;
    while((c<6) && (*addr == *idStr)) {
      ++addr;
      ++idStr;
      ++c;
    }
    if(c==6) {
      gamecard_slot = slot;
      gamecard_baseport = 0x10 + 0x10*slot;
      return;
    }
  }
#ifdef EMU
  gamecard_slot = 0;
  gamecard_baseport = 0x10;
#endif
  // gamecard_slot = 0;
  // gamecard_baseport = 0x10;
}

char check_internal_fire() {
  unsigned char b = 0;
  b = (joystick(1) & (MOVE_FIRE | MOVE_FIRE2)) !=0 ? 1 : 0;
  b |= (joystick(2) & (MOVE_FIRE | MOVE_FIRE2)) !=0 ? 2 : 0;
  return b;
}

char check_external_fire() {
//  if(gamecard_slot==4)
//    return 0;
  char b = 0;
  b = (inp(gamecard_baseport+2) & 0x10)==0 ? 4 : 0;
  b |= (inp(gamecard_baseport+3) & 0x10)==0 ? 8 : 0;

  return b;
}

void start_ai_snake() {
  DEBUG_COLOR_BORDER(red);
  snake_point sp;
  sp.addr = 0;
  do {
    sp.x = rand() % MAX_X;
    sp.y = rand() % MAX_Y;
  } while(get_screen_color(&sp) != black);
  ai_snake_head_point = &snake[MAX_PLAYERS][0];
  *ai_snake_head_point = sp;
  sp.addr = 0x8000 + (((u16_t)sp.y)<<7) + (sp.x>>1);
  snake_length[MAX_PLAYERS] = 1;
  snake_array_headpos[MAX_PLAYERS] = 0;
  snake_plays[MAX_PLAYERS] = true;
  char sdx = food_point.x - ai_snake_head_point->x;
  char sdy = food_point.y - ai_snake_head_point->y;
  snake_directions[MAX_PLAYERS] = 0;
/*
  unsigned char *dir = &snake_directions[MAX_PLAYERS];
  if(abs(sdx)>abs(sdy)) {
    if(sdx>0) {
      *dir = MOVE_RIGHT;
    } else {
      *dir = MOVE_LEFT;
    }
  } else {
    if(sdy>0) {
      *dir = MOVE_DOWN;
    } else {
      *dir = MOVE_UP;
    }
  }
*/
  DEBUG_COLOR_BORDER(black);
}

bool move_ai_snake() {
  char sdx = food_point.x - ai_snake_head_point->x;
  char sdy = food_point.y - ai_snake_head_point->y;
  unsigned char dx = abs(sdx);
  unsigned char dy = abs(sdy);
  unsigned char *cdir = &snake_directions[MAX_PLAYERS];
  unsigned char ndir = 0;
  snake_point aisp;
  unsigned char rotate_rounds = 0;
  unsigned char screen_color=black;
  
  if(dx>dy) {
    if((sdx > 0)) {
      ndir = MOVE_RIGHT;
    } else {
      ndir = MOVE_LEFT;
    }
  } else {
    if((sdy > 0)) {
      ndir = MOVE_DOWN;
    } else {
      ndir = MOVE_UP;
    }
  }
  unsigned pridx = 0;
  bool stay = true;
  while(ndir!=PREDEF_DIRECTIONS[pridx])
    ++pridx;

  do {
    aisp = *ai_snake_head_point;
    switch(ndir) {
      case MOVE_UP:
        if((*cdir == MOVE_DOWN) || (aisp.y==0)) {
          ndir = MOVE_RIGHT;
          ++pridx;
          continue;
        }
        --aisp.y;
        aisp.addr-=128;
        break;
      case MOVE_DOWN:
        if((*cdir == MOVE_UP) || (aisp.y == MAX_Y)) {
          ndir = MOVE_LEFT;
          ++pridx;
          continue;
        }
        ++aisp.y;
        aisp.addr+=128;
        break;
      case MOVE_LEFT:
        if((*cdir == MOVE_RIGHT) || (aisp.x==0)) {
          ndir = MOVE_UP;
          ++pridx;
          continue;
        }
        --aisp.x;
        if(aisp.x & 1)
          --aisp.addr;
        break;
      case MOVE_RIGHT:
        if((*cdir == MOVE_LEFT) || (aisp.x == MAX_X)) {
          ndir = MOVE_DOWN;
          ++pridx;
          continue;
        }
        ++aisp.x;
        if((aisp.x & 1) == 0)
          ++aisp.addr;
        break;
    }
    screen_color = get_screen_color(&aisp);
    
    if((screen_color!=black) && (screen_color!=white)) {
      ndir = PREDEF_DIRECTIONS[pridx & 0x03];
      ++pridx;
      continue;
    } else {
      stay = false;
    }
  } while (stay && ((++rotate_rounds)<5));
  if(rotate_rounds<5) {
    if(screen_color==white) {
      food_on_screen = false;
    }
    *cdir = ndir;
    return true;
  } else {
    snake_plays[MAX_PLAYERS] = false;
    u8_t pl = 0;
    u8_t *c = &SNAKE_COLORS[0];
    while((pl<4) && (*c != screen_color)) {
      ++pl;
      ++c;
    }
    if(pl<4) {
      score[pl]+=3;
      print_score(pl);
    }
    return false;
  }
}

void blink_players(bool visible, int players) {
  for(unsigned char i=0; i<MAX_PLAYERS; i++) {
    if(players & (1<<i))
      draw_point(visible?SNAKE_COLORS[i]:black, &snake[i][0]);
  }
}
 
char wait_for_players() {
  color_or_index c;
  c.color = white;
  tvc_set_ink(c);
  c.color = black;
  tvc_set_paper(c);
  tvc_ed_cpos(4,10);
  printf("Press fire\n");
  tvc_ed_cpos(4,12);
  printf("to start !");
  char joy_fired = 0;
  while(joy_fired == 0) {
    joy_fired = check_internal_fire();
    joy_fired |= check_external_fire();
#ifdef EMU
    joy_fired = 1;
#endif
  }
  tvc_ed_cpos(4,12);
  printf("to  join !");

  RESET_TIME_COUNTER();
  int cl=0;
  int lastCl = -100;
  int i = 0;
#ifdef EMU
#define COUNTER 2
#else
#define COUNTER 5
#endif
  while ( (cl < (COUNTER * 50)) && (joy_fired != 15)) {
    joy_fired |= check_internal_fire();
    joy_fired |= check_external_fire();
    tvc_ed_cpos(8,14);
    if(cl-lastCl>50) {
      tvc_ed_cpos(8,17);
      printf("%d", (COUNTER - cl/(50)));
      lastCl = cl;
    }
    blink_players((cl%10) < 7, joy_fired);
    cl = clock() >> 16;
  }
#ifdef EMU
  return 15;
#else
  return joy_fired;
#endif
}

void init_screen() {
  tvc_set_border(BORDERCOLOR);
  tvc_set_paper(black);
  tvc_set_vmode(VMODE_16C);
}

u8_t convert_to_joystick(u8_t ext_joy_value) {
#ifdef EMU
  ext_joy_value = 0xff;
#endif
  u8_t retVal = 0;
  retVal |= (ext_joy_value & 1) == 0 ? MOVE_UP    : 0;
  retVal |= (ext_joy_value & 2) == 0 ? MOVE_DOWN  : 0;
  retVal |= (ext_joy_value & 4) == 0 ? MOVE_LEFT  : 0;
  retVal |= (ext_joy_value & 8) == 0 ? MOVE_RIGHT : 0;
//   joy button is not relevant
//   retVal |= (ext_joy_value & 16) == 0 ? 16 : 0;
  return retVal;
}

u8_t save = 0;

void mapin_videoRAM() {
  save = *((u8_t *)0x0003);
  *((u8_t *)0x0003) = save & (~0x20);
  outp(2, save & (~0x20));
}

void mapout_videoRAM() {
  *((u8_t *)0x0003) = save;
  outp(2,save);
}

enum color get_screen_color(snake_point* point) {
  u8_t *videoRam;
  if(point->addr == 0) {
    videoRam = (u8_t *)0x8000;
    videoRam += ((unsigned short)point->y << 7) + (point->x >> 1);
    point->addr = videoRam;
  } else {
    videoRam = (u8_t *)point->addr;
  }

  if((point->x & 1) == 0) {
    return (*videoRam & 0b10101010) >> 1; 
  } else {
    return (*videoRam & 0b01010101);
  }
}

void draw_point(u8_t c, snake_point* point) {
  u8_t *videoRam;
  if(point->addr == 0) {
    videoRam = (u8_t *)0x8000;
    videoRam += (((unsigned short)point->y << 7) + (point->x >> 1));
  } else {
    videoRam = (u8_t *)point->addr;
  }

  if((point->x % 2) == 0) {
    c <<= 1;
    *videoRam = (*videoRam & 0b01010101) | c;
    videoRam += 64;
    *videoRam = (*videoRam & 0b01010101) | c;
  } else {
    *videoRam = (*videoRam & 0b10101010) | c;
    videoRam += 64;
    *videoRam = (*videoRam & 0b10101010) | c;
  }
}

void game_loop() {
  int players = 4;
  int round = 0;
  while(players>=1) {
    DEBUG_COLOR_BORDER(BORDERCOLOR);
    players = 0;
    RESET_TIME_COUNTER();
    int c = (int)(clock() >> 16) + 5 - speed;
    while(((int)(clock()>>16) <= c ) ) {
      // wait some screen to be drawn...
    };
    
    bool *sp = &snake_plays[0] - 1;

    char *sahp = &snake_array_headpos[0] - 1;
    char *sl = &snake_length[0] - 1;

    DEBUG_COLOR_BORDER(white);
    if(!food_on_screen) {
      DEBUG_COLOR_BORDER(blue);
      do {
        DEBUG_COLOR_BORDER(black);
        food_point.x = rand() % MAX_X;
        food_point.y = rand() % MAX_Y;
        food_point.addr = 0;
        DEBUG_COLOR_BORDER(white);
      } while( get_screen_color(&food_point) != black ) ;
      draw_point(white, &food_point);
      DEBUG_COLOR_BORDER(blue);
      food_on_screen = true;
    }
    char *sd = (&snake_directions[0]) - 1;
    for(unsigned char i = 0; i<MAX_PLAYERS+1; i++) {
//      tvc_set_border(SNAKE_COLORS[i]);
      DEBUG_SNAKECOLOR_BORDER(i);
      ++sahp;
      ++sl;
      ++sp;
      ++sd;
      if(!(*sp)) { // do not check player as he is not playing
        continue;
      }

      char jd = 0;
      if(i<2)
        jd = joystick(i+1);
      else if(i<MAX_PLAYERS)
        jd = convert_to_joystick(inp(gamecard_baseport+i));
      else if(!move_ai_snake())
        continue;

      if(jd!=0) {
        if(*sd & (MOVE_LEFT|MOVE_RIGHT)) {
          jd &= MOVE_UP|MOVE_DOWN;
        } else if(*sd & (MOVE_UP|MOVE_DOWN)) {
          jd &= MOVE_LEFT|MOVE_RIGHT;
        }
        // let's assume here, that opposite directions are not present in the jdirs at the same time
        // This is a forbidden joy result: (left, right and up)
        if(jd!=0) 
          *sd=jd;
      }

      snake_point *oldSnakeHead = &snake[i][*sahp];
      snake_point *snakeHead;

      ++(*sahp);
      if(*sahp == MAX_SNAKE_SIZE) {
        *sahp = 0;
        snakeHead = &snake[i][0];
      } else {
        snakeHead=oldSnakeHead+1;
      }

      *snakeHead = *oldSnakeHead;

      switch(*sd) {
        case MOVE_UP:
          snakeHead->y--;
          if(snakeHead->y == 255) {
            *sp = false;
            continue;
          }
          snakeHead->addr -= 128;
          break;
        case MOVE_DOWN:
          snakeHead->y++;
          if(snakeHead->y == MAX_Y) {
            *sp = false;
            continue;
          }
          snakeHead->addr += 128;
          break;
        case MOVE_LEFT:
          snakeHead->x--;
          if(snakeHead->x == 255) {
            *sp = false;
            continue;
          } else if (snakeHead->x & 0x01) {
            --(snakeHead->addr);
          }
          break;
        case MOVE_RIGHT:
          snakeHead->x++;
          if(snakeHead->x == MAX_X) {
            *sp = false;
            continue;
          } else if ((snakeHead->x & 0x01) == 0x00) {
            ++(snakeHead->addr);
          }
          break;
      }

      if(*sl<10) {
        ++(*sl);
      } else {
        int pos = *sahp - *sl;
        if(pos<0)
          pos += MAX_SNAKE_SIZE;
        draw_point(black,&snake[i][pos]);
      }
      enum color sCol = get_screen_color(snakeHead);
      if((sCol!=black) && (sCol!=white)) {
        *sp = false;
        continue;
      } else if(sCol == white) {
        food_on_screen=false;
        if(*sl < MAX_SNAKE_SIZE-1) {
          ++(*sl);
        }
        ++food_count;
        if(((food_count % 2) == 0) && (!snake_plays[MAX_PLAYERS])) {
          start_ai_snake();
        }
        if(i!=MAX_PLAYERS) {
          if((food_count == next_speed_change) && (speed!=5)) {
            ++speed;
            at_this_speed+=3;
            next_speed_change = food_count + at_this_speed;
          }
        } else {
          ++next_speed_change;
        }
        if(i<MAX_PLAYERS) {
          ++score[i];
          print_score(i);
        }
      }

      draw_point(SNAKE_COLORS[i], snakeHead);
      if(i!=MAX_PLAYERS) {
        players++;
      } else {
        ai_snake_head_point = snakeHead;
      }
    }
    round++;
    DEBUG_COLOR_BORDER(BORDERCOLOR);
  }
}

extern void snake_isr(void);
#asm
_snake_isr:
  push hl
  ld hl,(0x0b1d)
  inc hl
  ld (0x0b1d),hl
  pop hl
  out (0x07),a    ; acknowlegde interrupts in TVC
  ei              ; enable interrupts
  reti
#endasm

void main() {
  intrinsic_di();
  bpoke(0x0038, 195);              // POKE jump instruction (0xC3) at address 0x0038 (interrupt service routine entry)
  wpoke(0x0039, snake_isr);        // POKE isr address after the JP instruction
  intrinsic_ei();
  init_screen();
  find_slot();
  mapin_videoRAM();
  srand((unsigned int) (clock()>>16));
  while (true) {
    init_game();
    char players = wait_for_players();
    for(unsigned char i=0; i<4; i++) {
      snake_plays[i] = players & (1<<i);
    }
    tvc_cls();
    init_score();
    game_loop();
  }
  mapout_videoRAM();
}