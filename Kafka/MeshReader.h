#include <OpenMesh/Core/IO/Options.hh>
#include <OpenMesh/Core/IO/reader/BaseReader.hh>
#include <OpenMesh/Core/IO/reader/PLYReader.hh>
#include <OpenMesh/Core/IO/reader/OBJReader.hh>
#include <OpenMesh/Core/IO/importer/BaseImporter.hh>
#include "Mesh.h"
#include <boost\thread\mutex.hpp>
//=============================================================================
bool can_u_read2(const std::string& _filename, const std::string _extension);
//=============================================================================
bool read2(const std::string& _filename, OpenMesh::IO::BaseImporter& _bi, 
	OpenMesh::IO::Options& _opt);
//=============================================================================
int readMesh2(Mesh& _mesh, const string _filename, OpenMesh::IO::Options _ropt);
//=============================================================================
template <class Mesh>
bool
read_mesh2(Mesh&         _mesh,
const std::string&  _filename,
OpenMesh::IO::Options&  _opt,
bool                _clear = true)
{
	if (_clear) _mesh.clear();
	OpenMesh::IO::ImporterT<Mesh> importer(_mesh);
	return read2(_filename, importer, _opt);
}
//=============================================================================