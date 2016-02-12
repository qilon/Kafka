//*****************************************************************************
// Qi Liu Yin
// Recovering High Frequency Surface Details for Dynamic 4D Capture
// UCL - MSc Computer Graphics, Vision and Imaging
//*****************************************************************************
#include "Mesh.h"
//=============================================================================
int readMesh(Mesh& _mesh, const string _filename, OpenMesh::IO::Options _ropt)
{
	_mesh.request_vertex_normals();
	_mesh.request_vertex_colors();

	if (!OpenMesh::IO::read_mesh(_mesh, _filename, _ropt))
	{
		std::cout << "Could not read file: " << _filename << std::endl << std::endl;

		return -1;
	}

	return 0;
}
//=============================================================================
