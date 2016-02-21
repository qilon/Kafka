#include "MeshReader.h"
//=============================================================================
bool can_u_read2(const std::string& _filename, const std::string _extension)
{
	// get file extension
	std::string extension;
	std::string::size_type pos(_filename.rfind("."));

	if (pos != std::string::npos)
	{
		extension = _filename.substr(pos + 1, _filename.length() - pos - 1);

		std::transform(extension.begin(), extension.end(),
			extension.begin(), tolower);
	}

	// locate extension in extension string
	return (_extension.find(extension) != std::string::npos);
}
//=============================================================================
bool
read2(const std::string& _filename, OpenMesh::IO::BaseImporter& _bi, OpenMesh::IO::Options& _opt)
{
	if (can_u_read2(_filename, "ply"))
	{
		_bi.prepare();
		boost::mutex mutex;
		mutex.lock();
		bool ok = OpenMesh::IO::PLYReader().read(_filename, _bi, _opt);
		mutex.unlock();

		_bi.finish();
		return ok;
	}
	else
	{
		if (can_u_read2(_filename, "obj"))
		{
			_bi.prepare();
			boost::mutex mutex;
			mutex.lock();
			bool ok = OpenMesh::IO::OBJReader().read(_filename, _bi, _opt);
			mutex.unlock();

			_bi.finish();
			return ok;
		}
	}

	// All modules failed to read
	return false;
}
//=============================================================================
int readMesh2(Mesh& _mesh, const string _filename, OpenMesh::IO::Options _ropt)
{
	_mesh.request_face_normals();
	_mesh.request_vertex_normals();
	_mesh.request_vertex_colors();

	//if (!OpenMesh::IO::read_mesh(_mesh, _filename, _ropt))
	if (!read_mesh2(_mesh, _filename, _ropt))
	{
		std::cout << "Could not read file: " << _filename << std::endl << std::endl;

		return -1;
	}

	_mesh.update_normals();

	return 0;
}
//=============================================================================