#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>

#include "mos6507.h"
#include "mos6507_trace.h"
#include "mem.h"
#include "tia.h"
#include "pia.h"
#include "cart.h"
#include "gui.h"
#include "console.h"
#include "tas.h"



static mos6507_t cpu;
static mem_t mem;
static pia_t pia;
static tia_t tia;
static cart_t cart;

static bool debugger_break = false;
static bool vsync_break    = false;
static bool rdy_break      = false;
static char panic_msg[80];
static uint32_t frame_no;

static mos6507_t save_cpu;
static mem_t save_mem;
static pia_t save_pia;
static tia_t save_tia;
static cart_t save_cart;
static bool saved_state = false;



static bool debugger(void)
{
  char cmd[16];

  fprintf(stdout, "\n");
  while (1) {
    fprintf(stdout, "fr=%06d:sl=%03d:dot=%03d:pc=%04x> ",
      frame_no, tia.scanline, tia.dot, cpu.pc);

    if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
      if (feof(stdin)) {
        exit(EXIT_SUCCESS);
      }
      continue;
    }

    switch (cmd[0]) {
    case '?':
    case 'h':
      fprintf(stdout, "Commands:\n");
      fprintf(stdout, "  q - Quit\n");
      fprintf(stdout, "  h - Help\n");
      fprintf(stdout, "  c - Continue\n");
      fprintf(stdout, "  v - Continue until next VSYNC\n");
      fprintf(stdout, "  b - Continue until RDY released\n");
      fprintf(stdout, "  s - Step\n");
      fprintf(stdout, "  1 - Dump CPU Trace\n");
      fprintf(stdout, "  2 - Dump RAM\n");
      fprintf(stdout, "  3 - Dump PIA Info\n");
      fprintf(stdout, "  4 - Dump TIA Info\n");
      fprintf(stdout, "  5 - Dump Cartridge\n");
      break;

    case 'c': /* Continue */
      return false;

    case 'v': /* Continue until next VSYNC */
      vsync_break = true;
      return false;

    case 'b': /* Continue until RDY released */
      rdy_break = true;
      return false;

    case 's': /* Step */
      return true;

    case 'q': /* Quit */
      exit(EXIT_SUCCESS);
      break;

    case '1':
      mos6507_trace_dump(stdout);
      break;

    case '2':
      mem_dump(stdout, &mem, 0x0080, 0x00FF);
      break;

    case '3':
      pia_dump(stdout, &pia);
      break;

    case '4':
      tia_dump(stdout, &tia);
      break;

    case '5':
      cart_dump(stdout, &cart);
      mem_dump(stdout, &mem, 0xF000, 0xFFFF); /* Mapped address space. */
      break;

    default:
      continue;
    }
  }
}



static void sig_handler(int sig)
{
  (void)sig;
  debugger_break = true;
}



void panic(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vsnprintf(panic_msg, sizeof(panic_msg), format, args);
  va_end(args);

  debugger_break = true;
}



void debug(void)
{
  debugger_break = true;
}



void sync(void)
{
  /* Run PIA and TIA one CPU clock at a time: */
  while (cpu.cycles > 0) {
    pia_execute(&pia);
    tia_execute(&tia);
    tia_execute(&tia);
    tia_execute(&tia);
    cpu.cycles--;
  }
}



static void display_help(const char *progname)
{
  fprintf(stdout, "Usage: %s <options> [rom]\n", progname);
  fprintf(stdout, "Options:\n"
    "  -h        Display this help.\n"
    "  -d        Break into debugger on start.\n"
    "  -v        Disable SDL video.\n"
    "  -a        Disable SDL audio.\n"
    "  -c        Disable Curses console.\n"
    "  -s        Do not strip VBLANK scanlines in console.\n"
    "  -k        Disable colors in console.\n"
    "  -j NO     Use SDL joystick NO instead of 0.\n"
    "  -t FILE   Use CSV FILE as input for TAS.\n"
    "\n");
}



int main(int argc, char *argv[])
{
  int c;
  char *rom_filename = NULL;
  char *tas_filename = NULL;
  bool redraw_done;
  bool disable_video = false;
  bool disable_audio = false;
  bool disable_console = false;
  bool disable_vblank_strip = false;
  bool disable_colors = false;
  int joystick_no = 0;

  while ((c = getopt(argc, argv, "hdvacskj:t:")) != -1) {
    switch (c) {
    case 'h':
      display_help(argv[0]);
      return EXIT_SUCCESS;

    case 'd':
      debugger_break = true;
      break;

    case 'v':
      disable_video = true;
      break;

    case 'a':
      disable_audio = true;
      break;

    case 'c':
      disable_console = true;
      break;

    case 's':
      disable_vblank_strip = true;
      break;

    case 'k':
      disable_colors = true;
      break;

    case 'j':
      joystick_no = atoi(optarg);
      break;

    case 't':
      tas_filename = optarg;
      break;

    case '?':
    default:
      display_help(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (argc <= optind) {
    display_help(argv[0]);
    return EXIT_FAILURE;
  } else {
    rom_filename = argv[optind];
  }

  mos6507_trace_init();
  panic_msg[0] = '\0';

  signal(SIGINT, sig_handler);

  mem_init(&mem);
  pia_init(&pia, &mem);
  tia_init(&tia, &mem);
  cart_init(&cart, &mem);

  if (cart_load(&cart, rom_filename) != 0) {
    fprintf(stderr, "Unable to load cartridge ROM: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  if (tas_filename != NULL) {
    if (tas_init(tas_filename) != 0) {
      fprintf(stderr, "Failed to load TAS file: %s\n", tas_filename);
      return EXIT_FAILURE;
    }
  }

  if (gui_init(joystick_no, disable_video, disable_audio) != 0) {
    fprintf(stderr, "Failed to initialize SDL!\n");
    return EXIT_FAILURE;
  }

  if (! disable_console) {
    console_init(! disable_vblank_strip, disable_colors);
  }

  redraw_done = false;
  frame_no = 0;
  mos6507_reset(&cpu, &mem);
  while (1) {
    if (tia.rdy) {
      mos6507_trace_add(&cpu, &mem);
      mos6507_execute(&cpu, &mem);
    } else {
      /* CPU halted by RDY, but increment cycles: */
      cpu.cycles++;
    }

    /* Run TIA/PIA to catch up to CPU: */
    sync();

    if (rdy_break && tia.rdy) {
      rdy_break = false;
      debugger_break = true;
    }

    /* Redraw screen on vsync: */
    if (tia.vsync) {
      if (! redraw_done) {
        gui_update();
        console_update();
        tas_update();
        redraw_done = true;
        frame_no++;
        if (vsync_break) {
          vsync_break = false;
          debugger_break = true;
        }

        if (gui_save_state_requested()) {
          memcpy(&save_cpu,  &cpu,  sizeof(mos6507_t));
          memcpy(&save_mem,  &mem,  sizeof(mem_t));
          memcpy(&save_pia,  &pia,  sizeof(pia_t));
          memcpy(&save_tia,  &tia,  sizeof(tia_t));
          memcpy(&save_cart, &cart, sizeof(cart_t));
          saved_state = true;
        } else if (gui_load_state_requested() && saved_state) {
          memcpy(&cpu,  &save_cpu,  sizeof(mos6507_t));
          memcpy(&mem,  &save_mem,  sizeof(mem_t));
          memcpy(&pia,  &save_pia,  sizeof(pia_t));
          memcpy(&tia,  &save_tia,  sizeof(tia_t));
          memcpy(&cart, &save_cart, sizeof(cart_t));
        }
      }
    } else {
      redraw_done = false;
    }

    if (debugger_break) {
      console_pause();
      if (panic_msg[0] != '\0') {
        fprintf(stdout, "%s", panic_msg);
        panic_msg[0] = '\0';
      }
      debugger_break = debugger();
      if (! debugger_break) {
        console_resume();
      }
    }
  }

  return EXIT_SUCCESS;
}



