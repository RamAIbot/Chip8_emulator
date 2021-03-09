#include <stdio.h>
#include "SDL2/SDL.h"
#include "chip8.h"
#include <Windows.h>


const char keyboard_map[CHIP8_TOTAL_KEYS] = {
        SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7,
        SDLK_8, SDLK_9, SDLK_a, SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f
    };

int main(int argc,char** argv)
{
    if(argc < 2)
    {
        printf("You must provide a file to load\n");
        return -1;
    }

    const char *filename = argv[1];
    
    FILE *f = fopen(filename,"rb");
    if(!f)
    {
        printf("Cannot open file\n");
        return -1;
    }

    fseek(f,0,SEEK_END);

    long size = ftell(f);
    //printf("%d\n",size);
    fseek(f,0,SEEK_SET);

    char buf[size];
    
    int res = fread(buf,size,1,f);
    if(res!=1)
    {
        printf("Failed to read from fail \n");
        return -1;
    }
   // printf("%s\n",buf);

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow(
        EMULATOR_WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        CHIP8_WIDTH*CHIP8_WINDOW_MULTIPLIER,CHIP8_HEIGHT*CHIP8_WINDOW_MULTIPLIER,SDL_WINDOW_SHOWN
    );

    struct chip8 chip8;
    chip8_init(&chip8);
    chip8_load(&chip8,buf,size);//DONT miss to load or else random value comes in output of opcode
    chip8_keyboard_set_map(&chip8.keyboard,keyboard_map);
    
    //chip8.registers.V[0] = 0x00;
    //chip8_exec(&chip8,0xF00A);
    //printf("%x\n",chip8.registers.V[0]);
    //chip8_screen_draw_sprite(&chip8.screen,32,30,&chip8.memory.memory[0x00],5);
    //chip8_exec(&chip8,0x00E0);  //clear screen
    //chip8_exec(&chip8,0x1ff2);  //Jump address ff2
    //chip8.registers.V[0] = 0x22;  //if V[x] = kk Pc+=2
    //chip8_exec(&chip8,0x3022);
    //printf("%x\n",chip8.registers.PC);
    //printf("%x\n",chip8.registers.PC);
    //chip8.registers.delay_timer = 20;
    //chip8.registers.sound_timer = 30;

    //chip8_screen_set(&chip8.screen,0,0);
    // chip8.registers.V[0] = 50;
    // chip8_memory_set(&chip8.memory,50,'R');
    // printf("%c\n",chip8_memory_get(&chip8.memory,50));

    // chip8.registers.SP = 0;
    // chip8_stack_push(&chip8,0xff);
    // chip8_stack_push(&chip8,0xaa);

    // printf("%x\n",chip8_stack_pop(&chip8));
    // printf("%x\n",chip8_stack_pop(&chip8));

    // chip8_keyboard_down(&chip8.keyboard,0x0f);
    // bool is_down = chip8_keyboard_is_down(&chip8.keyboard,0x0f);
    // printf("%i\n",is_down);

    // chip8_keyboard_up(&chip8.keyboard,0x0f);
    // bool is_down1 = chip8_keyboard_is_down(&chip8.keyboard,0x0f);
    // printf("%i\n",is_down1);
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window,-1,SDL_TEXTUREACCESS_TARGET);

    while(1)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                    goto out;
                    break;

                case SDL_KEYDOWN:
                {
                    char key = event.key.keysym.sym;
                    int vkey = chip8_keyboard_map(&chip8.keyboard,key);
                    if(vkey!=-1)
                    {
                        chip8_keyboard_down(&chip8.keyboard,vkey);
                    }
                }
                break;

                case SDL_KEYUP:
                {
                    char key = event.key.keysym.sym;
                    int vkey = chip8_keyboard_map(&chip8.keyboard,key);
                    if(vkey!=-1)
                    {
                        chip8_keyboard_up(&chip8.keyboard,vkey);
                    }
                }
                break;
            }
            
        }

        SDL_SetRenderDrawColor(renderer,0,0,0,0);
        SDL_RenderClear(renderer);
        
        SDL_SetRenderDrawColor(renderer,255,255,255,0);

        for(int x=0;x<CHIP8_WIDTH;x++)
        {
            for(int y=0;y<CHIP8_HEIGHT;y++)
            {
                if(chip8_screen_is_set(&chip8.screen,x,y))
                {
                    SDL_Rect r;
                    r.x= x*CHIP8_WINDOW_MULTIPLIER;
                    r.y= y*CHIP8_WINDOW_MULTIPLIER;
                    r.w= CHIP8_WINDOW_MULTIPLIER;
                    r.h= CHIP8_WINDOW_MULTIPLIER;
                    SDL_RenderFillRect(renderer,&r);
                }
            }
        }
        
        SDL_RenderPresent(renderer);

        if(chip8.registers.delay_timer > 0)
        {
            Sleep(1);
            chip8.registers.delay_timer -=1;
            //printf("Delay!\n");
        }

        if(chip8.registers.sound_timer > 0)
        {
            Beep(15000,10*chip8.registers.sound_timer);
            chip8.registers.sound_timer = 0;
        }

        unsigned short opcode = chip8_memory_get_short(&chip8.memory,chip8.registers.PC);
        chip8.registers.PC += 2;
        chip8_exec(&chip8,opcode);
        
       // printf("%x\n",opcode);

    }

    out:
    SDL_DestroyWindow(window);
    return 0;
}