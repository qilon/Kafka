//*****************************************************************************
// Qi Liu Yin
// Recovering High Frequency Surface Details for Dynamic 4D Capture
// UCL - MSc Computer Graphics, Vision and Imaging
//*****************************************************************************
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <iostream>
//=============================================================================
using namespace OpenMesh;
using namespace std;
//=============================================================================
struct Traits
{
	/// The default coordinate type is OpenMesh::Vec3f.
	typedef OpenMesh::Vec3f  Point;

	/// The default normal type is OpenMesh::Vec3f.
	typedef OpenMesh::Vec3f  Normal;

	/// The default 1D texture coordinate type is float.
	typedef float  TexCoord1D;
	/// The default 2D texture coordinate type is OpenMesh::Vec2f.
	typedef OpenMesh::Vec2f  TexCoord2D;
	/// The default 3D texture coordinate type is OpenMesh::Vec3f.
	typedef OpenMesh::Vec3f  TexCoord3D;

	/// The default texture index type
	typedef int TextureIndex;

	/// The default color type is OpenMesh::Vec3f.
	typedef OpenMesh::Vec3f Color;

#ifndef DOXY_IGNORE_THIS
	VertexTraits{};
	HalfedgeTraits{};
	EdgeTraits{};
	FaceTraits{};
#endif

	VertexAttributes(0);
	HalfedgeAttributes(OpenMesh::Attributes::PrevHalfedge);
	EdgeAttributes(0);
	FaceAttributes(0);
};
//=============================================================================
typedef OpenMesh::PolyMesh_ArrayKernelT<Traits> Mesh;
//=============================================================================
int readMesh(Mesh& _mesh, const std::string _filename, OpenMesh::IO::Options _ropt);
//=============================================================================
int writeMesh(Mesh& _mesh, const std::string _filename, OpenMesh::IO::Options _wopt);
//=============================================================================
