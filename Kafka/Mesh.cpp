//*****************************************************************************
// Qi Liu Yin
// Recovering High Frequency Surface Details for Dynamic 4D Capture
// UCL - MSc Computer Graphics, Vision and Imaging
//*****************************************************************************
#include "Mesh.h"
//=============================================================================
int readMesh(Mesh& _mesh, const string _filename, OpenMesh::IO::Options _ropt)
{
	_mesh.request_face_normals();
	_mesh.request_vertex_normals();
	_mesh.request_vertex_colors();

	if (!OpenMesh::IO::read_mesh(_mesh, _filename, _ropt))
	{
		cout << "Could not read file: " << _filename << endl << endl;

		return -1;
	}

	_mesh.update_normals();

	return 0;
}
//=============================================================================
int writeMesh(const Mesh& _mesh, const string _filename,
	const OpenMesh::IO::Options _wopt)
{
	if (!OpenMesh::IO::write_mesh(_mesh, _filename, _wopt))
	{
		cout << "Could not write file: " << _filename << endl << endl;

		return -1;
	}

	return 0;
}
//=============================================================================