#include "Viewer.h"
//=============================================================================
int main(int argc, char **argv)
{
	Viewer::initialize(&argc, argv);
	Viewer::run();
	Viewer::destroy();

	return 0;
}
//=============================================================================