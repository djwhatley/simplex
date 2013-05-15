#include <ncurses.h>
#include <errno.h>
#include "../include/lc3sim.h"
#include "../include/lc3gui.h"

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("Bad argument! Just give me a filename of a compiled assembly program.\n");
		return -EINVAL;
	}

	FILE* program;
	if (!(program = fopen(argv[1], "r")))
	{
		printf("Bad argument! File not found.\n");
		return -EINVAL;
	}

	read_program(program);
	pc = 0x3000;
	running = 1;

	int ch;
	initialize();

	step_forward();
	refreshall();
	while(ch != KEY_F(1))
	{
		ch = getch();
		switch (ch) {
		case KEY_F(5):
			step_forward();
			break;
		case KEY_F(6):
			run_program();
			break;
		case KEY_F(2):
			break;
		}
		refreshall();
	}
quit:
	curs_set(1);
	endwin();
	return 0;
}

void initialize()
{
	initscr();
	cbreak();
	curs_set(0);
	keypad(stdscr, TRUE);

	refresh();

	WINDOW* mem_win, *reg_win, *cns_win, *dbg_win;

	mem_win = create_win(LINES-DEBUGWIN_HEIGHT, COLS-REGWIN_WIDTH, 0, 0);
	reg_win = create_win(REGWIN_HEIGHT, REGWIN_WIDTH, 0, COLS-REGWIN_WIDTH);
	cns_win = create_win(LINES-REGWIN_HEIGHT-DEBUGWIN_HEIGHT, REGWIN_WIDTH, REGWIN_HEIGHT, COLS-REGWIN_WIDTH);
	dbg_win = create_win(DEBUGWIN_HEIGHT, COLS, LINES-DEBUGWIN_HEIGHT, 0);

	wprintw(mem_win, "Memory Viewer");
	wprintw(reg_win, "Registers");
	wprintw(cns_win, "Console");
	wprintw(dbg_win, "Debugging");

	refreshall();
}

WINDOW* create_win(int height, int width, int y, int x)
{
	WINDOW* win;

	win = newwin(height, width, y, x);
	box(win, 0, 0);

	wrefresh(win);

	windows[windex] = win;
	windex++;

	return win;
}

void refreshall()
{
	int i;
	update_memwin();
	update_regwin();
	for (i=0; i<windex; i++)
		wrefresh(windows[i]);
	refresh();
}


void update_memwin()
{
	int i;
	mem_index = pc;
	for (i; i<LINES-DEBUGWIN_HEIGHT-WINDOW_PADDING*2; i++)
	{
		short addr = mem_index-(LINES-DEBUGWIN_HEIGHT-WINDOW_PADDING)/2+i;
		short curr = mem[addr];
		char binstring[20];
		char disasmstr[35];
		hex_to_binstr(curr, binstring);
		disassemble_to_str(curr, disasmstr);
		if (pc == addr)
			wattron(MEMWIN, A_STANDOUT);
		int c;
		for (c=0; c<COLS-REGWIN_WIDTH-WINDOW_PADDING*2; c++)
			mvwprintw(MEMWIN, i+WINDOW_PADDING, c+WINDOW_PADDING, " ");
		mvwprintw(MEMWIN, i+WINDOW_PADDING, WINDOW_PADDING, "x%.4hx\t x%.4hx\t %.5d\t %s\t %s", addr, curr, curr, binstring, disasmstr);
		wattroff(MEMWIN, A_STANDOUT);
	}
}

void update_regwin()
{
	int i;
	for (i=0; i<4; i++)
		mvwprintw(REGWIN, i+WINDOW_PADDING, WINDOW_PADDING, "R%d: x%.4hx | R%d: x%.4hx", 2*i, regfile[2*i], 2*i+1, regfile[2*i+1]);
	mvwprintw(REGWIN, WINDOW_PADDING+6, WINDOW_PADDING, "PC: x%.4hx", pc);
	mvwprintw(REGWIN, WINDOW_PADDING+7, WINDOW_PADDING, "IR: x%.4hx", ir);
	mvwprintw(REGWIN, WINDOW_PADDING+8, WINDOW_PADDING, "CC: x%.4hx", cc);
}

void hex_to_binstr(short hex, char* buffer)
{
	char* strings[16] = { "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111" };

	int a = (hex & 0xF000) >> 12;
	int b = (hex & 0x0F00) >> 8;
	int c = (hex & 0x00F0) >> 4;
	int d = (hex & 0x000F) >> 0;

	sprintf(buffer, "%s %s %s %s", strings[a], strings[b], strings[c], strings[d]);
}