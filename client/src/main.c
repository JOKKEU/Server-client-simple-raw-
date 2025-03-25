#include "../hd/client.h"


int main(int argc, char** argv)
{
	if (client() != 0) {return EXIT_FAILURE;}
	return EXIT_SUCCESS;
}
