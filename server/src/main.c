#include "../hd/server.h"


int main(int argc, char** argv)
{
	if (server() != 0) {return EXIT_FAILURE;}
	return EXIT_SUCCESS;
}
