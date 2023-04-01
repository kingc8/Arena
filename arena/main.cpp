#include "arena.hpp"

int main()
{
	arena win(1480, 700, "Arena");
	win.show(); // This runs before anything else.
	return(Fl::run());
}
