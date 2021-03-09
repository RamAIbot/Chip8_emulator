#include "chip8.h"
#include <memory.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include "SDL2/SDL.h"

const char chip8_default_character_set[] = {
    0xF0,0x90,0x90,0x90,0xF0,
    0x20,0x60,0x20,0x20,0x70,
    0xF0,0x10,0xF0,0x80,0xF0,
    0xF0,0x10,0xF0,0x10,0xF0,
    0x90,0x90,0xF0,0x10,0x10,
    0xF0,0x80,0xF0,0x10,0xF0,
    0xF0,0x80,0xF0,0x90,0xF0,
    0xF0,0x10,0x20,0x40,0x40,
    0xF0,0x90,0xF0,0x90,0xF0,
    0xF0,0x90,0xF0,0x10,0xF0,
    0xF0,0x90,0xF0,0x90,0x90,
    0xE0,0x90,0xE0,0x90,0xE0,
    0xF0,0x80,0x80,0x80,0xF0,
    0xE0,0x90,0x90,0x90,0xE0,
    0xF0,0x80,0xF0,0x80,0xF0,
    0xF0,0x80,0xF0,0x80,0x80

};

void chip8_init(struct chip8 *chip8)
{
    memset(chip8,0,sizeof(struct chip8));
    memcpy(&chip8->memory.memory,chip8_default_character_set,sizeof(chip8_default_character_set));
}

static char chip8_wait_for_key_press(struct chip8 *chip8)
{
    SDL_Event event;
    while(SDL_WaitEvent(&event))
    {
        
        if(event.type != SDL_KEYDOWN)
            continue;
        char c = event.key.keysym.sym;
        char chip8_key = chip8_keyboard_map(&chip8->keyboard,c);
        if(chip8_key != -1)
            return chip8_key;
    }
    return -1;
}

void chip8_exec_extended_eight(struct chip8 *chip8,unsigned short opcode)
{
    unsigned short nnn = opcode & 0x0fff;
    unsigned char x = (opcode >> 8) & 0x000f;
    unsigned char y = (opcode >> 4) & 0x000f;
    unsigned char kk = opcode & 0x00ff;
    unsigned short tmp = 0;

    unsigned char final_four_bytes = opcode & 0x000f;
    switch(final_four_bytes)
    {
        //8xy0 Set Vx = Vy
        case 0x00:
            chip8->registers.V[x] = chip8->registers.V[y];
            break;
        
        //8xy1 - Vx = Vx Or Vy
        case 0x01:
            chip8->registers.V[x] = chip8->registers.V[x] | chip8->registers.V[y];
            break;

        //8xy2 - Vx = Vx And Vy
        case 0x02:
            chip8->registers.V[x] = chip8->registers.V[x] & chip8->registers.V[y];
            break;

        //8xy3 - V[x] = V[x] XOR V[y]
        case 0x03:
            chip8->registers.V[x] = chip8->registers.V[x] ^ chip8->registers.V[y];
            break;

        //8xy4 - V[x] = V[x] + V[y] and if res>255(8bits) VF = 1
        case 0x04:
            tmp = chip8->registers.V[x] + chip8->registers.V[y];
            chip8->registers.V[0x0f] = false;
            if(tmp > 0xff)
            {
                chip8->registers.V[0x0f] = true;
            }

            chip8->registers.V[x] = tmp;
            break;

        //8xy5 - V[x] = V[x] - V[y],VF = 1 if V[x]>V[y]
        case 0x05:
            chip8->registers.V[0x0f] = false;
            if(chip8->registers.V[x] > chip8->registers.V[y])
            {
                chip8->registers.V[0x0f] = true;
            }

            chip8->registers.V[x] = chip8->registers.V[x] - chip8->registers.V[y];
            break;

        //8xy6 - Set V[x] = V[x] SHR 1. If LSB of V[x]=1, VF=1 and V[x]/=2
        case 0x06:
            chip8->registers.V[0x0f] = chip8->registers.V[x] & 0x01;
            chip8->registers.V[x] /= 2;
            break;
        
        //8xy7 - V[x] = V[y] - V[x] VF = 1 if V[y]>V[x]
        case 0x07:
            chip8->registers.V[0x0f] = chip8->registers.V[y] > chip8->registers.V[x];

            chip8->registers.V[x] = chip8->registers.V[y] - chip8->registers.V[x];
            break;

        //8yxE - set V[x] = Vx SHL 1 (if MSB of V[x]=1 set VF =1 and then V[x]*=2)
        case 0x0E:
            chip8->registers.V[0x0f] = chip8->registers.V[x] * 0b10000000;
            chip8->registers.V[x] *= 2;
            break;

    }
}

static void chip8_exec_extended_F(struct chip8 *chip8,unsigned short opcode)
{
    unsigned short nnn = opcode & 0x0fff;
    unsigned char x = (opcode >> 8) & 0x000f;
    unsigned char y = (opcode >> 4) & 0x000f;
    unsigned char kk = opcode & 0x00ff;
    unsigned char n = opcode & 0x000f;

    
    switch(opcode & 0x00ff)
    {

        //Fx07 - Set V[x] = delay timer value
        case 0x07:
            chip8->registers.V[x] = chip8->registers.delay_timer; 
            break;

        //Fx0A - Wait for key press and store the valie of key in V[x]. All executions stops until keypress
        case 0x0A:
            {
            char pressed_key = chip8_wait_for_key_press(chip8);
            chip8->registers.V[x] = pressed_key;
            }
            break;

        //Fx15 - Set delay_time = V[x]
        case 0x15:
            chip8->registers.delay_timer = chip8->registers.V[x];
            break;

        //Fx18 - Set Sound timer = V[x]
        case 0x18:
            chip8->registers.sound_timer = chip8->registers.V[x];
            break;

        //Fx1E - Set I = I + V[x]
        case 0x1E:
            chip8->registers.I = chip8->registers.I + chip8->registers.V[x];
            break;

        //Fx29 - Set I = location of sprite for digit V[x]
        case 0x29:
            chip8->registers.I = chip8->registers.V[x] * 5;
            break;
        
        //Fx33 - Store BCD representation of V[x] in memory locations I, I+1, I+2
        case 0x33:
            {
                unsigned char hundereds = chip8->registers.V[x] / 100;
                unsigned char tenths = chip8->registers.V[x] /10 %10;
                unsigned char ones = chip8->registers.V[x] % 10;

                chip8_memory_set(&chip8->memory,chip8->registers.I,hundereds);
                chip8_memory_set(&chip8->memory,chip8->registers.I+1,tenths);
                chip8_memory_set(&chip8->memory,chip8->registers.I+2,ones);

            }
            break;

        //Fx55  - Store registers V[0] through V[x] in memory location starting at loation I
        case 0x55:
            {
                for(int i=0;i<=x;i++)
                {
                    chip8_memory_set(&chip8->memory,chip8->registers.I+i,chip8->registers.V[i]);
                }
            }
            break;

        //Fx65 - Read registers V[0] through V[x] in memory starting at location I
        case 0x65:
            {
                for(int i=0;i<=x;i++)
                {
                    chip8->registers.V[i] = chip8_memory_get(&chip8->memory,chip8->registers.I+i);
                }
            }
            break;
    }
}

static void chip8_exec_extended(struct chip8 *chip8,unsigned short opcode)
{
    unsigned short nnn = opcode & 0x0fff;
    unsigned char x = (opcode >> 8) & 0x000f;
    unsigned char y = (opcode >> 4) & 0x000f;
    unsigned char kk = opcode & 0x00ff;
    unsigned char n = opcode & 0x000f;

    switch(opcode & 0xf000)
    {
        //1nnn - JP addr, Jumps to location nnn.PC =nnn
        case 0x1000:
            chip8->registers.PC = nnn;
            break;
        
        //2nnn -  CALL addr,Call subroutine at nnn, Puts the PC in top of stack and sets PC = nnn;
        case 0x2000:
            chip8_stack_push(chip8,chip8->registers.PC);
            chip8->registers.PC = nnn;
            break;

        //3xkk - Skip next instruction if Vx = kk
        case 0x3000:
            if(chip8->registers.V[x] == kk)
            {
                chip8->registers.PC += 2;
            }
            break;

        //4xkk - Skip next instruction if Vx!=kk
        case 0x4000:
            if(chip8->registers.V[x]!=kk)
            {
                chip8->registers.PC +=2;
            }
            break;

        //5xy0 - Skip next instruction if Vx = Vy
        case 0x5000:
            if(chip8->registers.V[x] == chip8->registers.V[y])
            {
                chip8->registers.PC += 2;
            }
            break;

        //6xkk - Set Vx = kk
        case 0x6000:
            chip8->registers.V[x] = kk;
            break;

        //7xkk - ADD Vx,byte (Vx = Vx + kk)
        case 0x7000:
            chip8->registers.V[x] += kk;
            break;

        case 0x8000:
            chip8_exec_extended_eight(chip8,opcode);
            break;

        //9xy0 - Skip next instrution if V[x]!=V[y]
        case 0x9000:
            if(chip8->registers.V[x] != chip8->registers.V[y])
            {
                chip8->registers.PC += 2;
            }
            break;
        //Annn - Set I = nnn
        case 0xA000:
            chip8->registers.I = nnn;
            break;

        //Bnnn - PC = nnn + V[0]
        case 0xB000:
            chip8->registers.PC = chip8->registers.V[0] + nnn;
            break;

        //Cxkk - V[x] = Randoom byte (0 to 255) AND kk
        case 0xC000:
            srand(clock());
            chip8->registers.V[x] = (rand()%255) & kk;
            break;

        //Dxyn - Draw n byte sprite starting at memory location I at (V[x],V[y])
        //Set VF for collision (sprites are XORed)
        case 0xD000:
           {
           const char *sprite = &chip8->memory.memory[chip8->registers.I]; 
           chip8->registers.V[0x0f] = chip8_screen_draw_sprite(&chip8->screen,chip8->registers.V[x],chip8->registers.V[y],sprite,n);
           }
        break;

        
        case 0xE000:
        {
            switch(opcode & 0x00ff){

                //Ex9E - Skip next instruction if key with value of V[x] is pressed
                case 0x9E:
                    {
                        if(chip8_keyboard_is_down(&chip8->keyboard,chip8->registers.V[x]))
                        {
                            chip8->registers.PC += 2;
                        }
                    }
                    break;

                //ExA1 - Skip next instruction if key with value of V[x] is not pressed
                case 0xA1:
                    {
                        if(!chip8_keyboard_is_down(&chip8->keyboard,chip8->registers.V[x]))
                        {
                            chip8->registers.PC += 2;
                        }
                    }
                    break;
            }
        }

        case 0xF000:
        {
            chip8_exec_extended_F(chip8,opcode);
        }
        break;




        


    }
}

void chip8_exec(struct chip8 *chip8,unsigned short opcode)
{
    switch(opcode){
        //CLS => clear the display
        case 0x00E0:
            chip8_screen_clear(&chip8->screen);
            break;
        
        //RET => Return from subroutine. Sets PC to top of stack and subtracts 1 from stack pointer
        case 0x00EE:
            chip8->registers.PC = chip8_stack_pop(chip8);
            break;

        default:
            chip8_exec_extended(chip8,opcode);
    }
}

void chip8_load(struct chip8 *chip8,const char *buf,size_t size)
{
    assert(size + CHIP8_PROGRAM_LOAD_ADDRESS < CHIP8_MEMORY_SIZE);
    memcpy(&chip8->memory.memory[CHIP8_PROGRAM_LOAD_ADDRESS],buf,size);
    chip8->registers.PC = CHIP8_PROGRAM_LOAD_ADDRESS;
}