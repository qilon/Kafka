#include "Viewer.h"

#include "use_nvidia_gpu.h"

//=============================================================================
int main(int argc, char **argv)
{
	Viewer::initialize(&argc, argv);
	Viewer::run();
	Viewer::destroy();

	return 0;
}
//=============================================================================