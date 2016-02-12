//*****************************************************************************
// Qi Liu Yin
// Recovering High Frequency Surface Details for Dynamic 4D Capture
// UCL - MSc Computer Graphics, Vision and Imaging
//*****************************************************************************
#include "Viewer.h"
//=============================================================================
/**** CONSTANTS****/

/* PERSPECTIVE PROPERTIES */
const GLfloat Viewer::ZOOM_INCR = 5.f;

/* GLUI CONTROL PARAMETERS */
const float Viewer::TRANSLATION_SPEED = .5f;
const float Viewer::ZOOM_SPEED = .5f;
const float Viewer::ROTATION_SPIN_FACTOR = .2f;

const char* Viewer::FRAME_TEXT = "Frame: ";
const char* Viewer::PLAY_TEXT = "Play";
const char* Viewer::PAUSE_TEXT = "Pause";

const double Viewer::FRAME_FREQ = 1000.f / 30.f;

const int Viewer::PLAY_BUTTON_WIDTH = 300;
//=============================================================================
/**** VARIABLES ****/

parameters::Parameters Viewer::params;

OpenMesh::IO::Options Viewer::ropt;

/* MAIN VARIABLES */
vector<vector<Mesh>> Viewer::meshes;
int Viewer::curr_frame = 1;
bool Viewer::play = false;
clock_t Viewer::last_time;

VectorXf Viewer::sh_coeff;
MatrixXf* Viewer::sh_functions;
MatrixXf Viewer::albedo;
VectorXf* Viewer::shading;

//MatrixXf** Viewer::intensities = nullptr;
//VectorXi Viewer::mode;

/* VIEW VARIABLES */
GLfloat Viewer::eye[3] = { 0.f, 0.f, params.eye[2] };
GLfloat Viewer::aspectRatio = 1.f;

/* ROTATION AND TRANSLATION MATRIXES*/
float Viewer::translation[3] = { 0.f, 0.f, 0.f };
vector<vector<GLfloat>> Viewer::meshes_center;

/* MOUSE CONTROL VARIABLES */
int Viewer::moving = 0;
int Viewer::beginx = 0;
int Viewer::beginy = 0;
float Viewer::anglex = 0.f;
float Viewer::angley = 0.f;

/* GLUI COMPONENTS */
GLUI* Viewer::glui = nullptr;
int Viewer::window_id;
GLUI_Scrollbar* Viewer::glui_frame_scroll = nullptr;
GLUI_StaticText* Viewer::glui_frame_text = nullptr;
GLUI_Button* Viewer::glui_play_button = nullptr;
//=============================================================================
void Viewer::initGLUT(int *argc, char **argv)
{
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE | GLUT_ALPHA);
	glutInitWindowSize(params.window_width, params.window_height);

	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - params.window_width) / 2,
		10);

	window_id = glutCreateWindow(params.window_title.c_str());

	// Uncomment to enable transparencies
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);

	glutDisplayFunc(display);
	glutMotionFunc(motion);

	glewInit();

	/* Enable a single OpenGL light. */
	//glLightfv(GL_LIGHT0, GL_POSITION, LIGHT_POSITION);
	//glEnable(GL_COLOR_MATERIAL);
	//glLightfv(GL_LIGHT0, GL_AMBIENT, LIGHT_AMBIENT);
	//glEnable(GL_LIGHT0);
	//glEnable(GL_LIGHTING);
	glDisable(GL_LIGHTING);

	glClearColor(params.background_colour[0], params.background_colour[1],
		params.background_colour[2], params.background_colour[3]);

	/* Use depth buffering for hidden surface elimination. */
	glEnable(GL_DEPTH_TEST);

	/* Anti-aliasing*/
	glEnable(GL_MULTISAMPLE);

	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}
//=============================================================================
void Viewer::initGLUI(void)
{
	glui = GLUI_Master.create_glui_subwindow(window_id,
		GLUI_SUBWINDOW_BOTTOM);
	glui->set_main_gfx_window(window_id);

	GLUI_Master.set_glutReshapeFunc(reshape);
	GLUI_Master.set_glutKeyboardFunc(key);
	GLUI_Master.set_glutSpecialFunc(NULL);
	GLUI_Master.set_glutMouseFunc(mouse);
	GLUI_Master.set_glutIdleFunc(idle);

	initGLUIComponents();
}
//=============================================================================
/*
 * Function that initialize the GLUI components
 * Adds the following elements:
 * - list of feature spinners from the info in pca
 * - translation tool
 * - zoom tool
 * - rotation tool
 * - check for showing or hiding guidance circles
 */
void Viewer::initGLUIComponents(void)
{
	GLUI_Panel* glui_panel = glui->add_panel("", GLUI_PANEL_NONE);

	glui_frame_scroll = new GLUI_Scrollbar(glui_panel, "Scrollbar", 
		GLUI_SCROLL_HORIZONTAL, GLUI_SCROLL_INT, -1, updateFrame);
	glui_frame_scroll->set_int_limits(1, params.n_frames, 1);
	glui_frame_scroll->set_w(glutGet(GLUT_WINDOW_WIDTH) - PLAY_BUTTON_WIDTH);
	glui_frame_scroll->set_int_limits(1, params.n_frames);

	glui->add_column_to_panel(glui_panel, 0);

	glui_frame_text = new GLUI_StaticText(glui_panel, "");
	glui_frame_text->set_alignment(GLUI_ALIGN_CENTER);
	updateFrameText();

	glui->add_column_to_panel(glui_panel, 0);

	glui_play_button = new GLUI_Button(glui_panel, PLAY_TEXT, -1, updatePlay);
}
//=============================================================================
void Viewer::updateFrame(int state)
{
	curr_frame = glui_frame_scroll->get_int_val();
	updateFrameText();
}
//=============================================================================
void Viewer::updateFrameText()
{
	string text = FRAME_TEXT + to_string(curr_frame);
	glui_frame_text->set_text(text.c_str());
}
//=============================================================================
void Viewer::updatePlay(int state)
{
	play = !play;
	if (play)
	{
		glui_play_button->set_text(PAUSE_TEXT);
		glui_play_button->set_name(PAUSE_TEXT);
	}
	else
	{
		glui_play_button->set_text(PLAY_TEXT);
		glui_play_button->set_name(PLAY_TEXT);

		if (curr_frame == params.n_frames)
			curr_frame = 1;
	}
}
//=============================================================================
void Viewer::display(void)
{
	clock_t curr_time = clock();
	double diff_time = (curr_time - last_time) / (double)(CLOCKS_PER_SEC / 1000);

	if (play && diff_time >= FRAME_FREQ && curr_frame<params.n_frames)
	{
		curr_frame++;
		glui_frame_scroll->set_int_val(curr_frame);
		updateFrameText();
		last_time = curr_time;

		if (curr_frame == params.n_frames)
		{
			updatePlay(0);
		}
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(aspectRatio * params.frustum_left, aspectRatio * params.frustum_right, 
		aspectRatio * params.frustum_bottom, aspectRatio * params.frustum_top, 
		aspectRatio * params.frustum_near, 
		aspectRatio * params.frustum_far);

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	gluLookAt(eye[0], eye[1], eye[2],	/* eye */
		eye[0], eye[1], 1.f,	/* center */
		0.f, 1.f, 0.f);	/* up is in positive Y direction */
	
	//glTranslatef(translation[0], translation[1], translation[2]);

	for (int i = 0; i < params.n_meshes; i++)
	{
		glPushMatrix();
		//glTranslatef(meshes_center[i][0], meshes_center[i][1],
		//	meshes_center[i][2]);
		drawModel(i, curr_frame - 1);
		glPopMatrix();
	}

	glutSwapBuffers();
}
//=============================================================================
void Viewer::reshape(int x, int y)
{
	int tx, ty, tw, th;
	GLUI_Master.get_viewport_area(&tx, &ty, &tw, &th);
	glViewport(tx, ty, tw, th);

	aspectRatio = (float)tw / (float)th;

	glui_frame_scroll->set_w(glutGet(GLUT_WINDOW_WIDTH) - PLAY_BUTTON_WIDTH);

	glutPostRedisplay();
}
//=============================================================================
void Viewer::mouse(int button, int state, int x, int y)
{
	// Left button pressed
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		moving = 1;
		beginx = x;
		beginy = y;
	}
	// Left button released
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		moving = 0;
	}
	// Wheel scroll down
	if (button == 3 && state == GLUT_DOWN) {
		zoom(-ZOOM_INCR);
	}
	// Wheel scroll up
	if (button == 4 && state == GLUT_DOWN) {
		zoom(ZOOM_INCR);
	}
}
//=============================================================================
void Viewer::motion(int x, int y)
{
	if (moving) {

		anglex = anglex + (x - beginx);
		angley = angley + (y - beginy);

		beginx = x;
		beginy = y;

		glutPostRedisplay();
	}
}
//=============================================================================
/*
* Controls:
* R/F -> Zoom in/out
* C   -> Show/hide guidance circles
*/
void Viewer::key(unsigned char key, int x, int y) {
	switch (key) {
	case UPPER_R:
		zoom(ZOOM_INCR);
		break;
	case LOWER_R:
		zoom(ZOOM_INCR);
		break;
	case UPPER_F:
		zoom(-ZOOM_INCR);
		break;
	case LOWER_F:
		zoom(-ZOOM_INCR);
		break;
	case UPPER_C:
		break;
	case LOWER_C:
		break;
	default:
		break;
	}
	glutPostRedisplay();
}
//=============================================================================
void Viewer::idle(void)
{
	/* According to the GLUT specification, the current window is
	undefined during an idle callback.  So we need to explicitly change
	it if necessary */
	if (glutGetWindow() != window_id)
		glutSetWindow(window_id);

	glutPostRedisplay();
}
//=============================================================================
void Viewer::drawModel(int _mesh_idx, int _frame_idx)
{
	Mesh* mesh2draw = &meshes[_mesh_idx][_frame_idx];

	//glTranslatef(-meshes_rotation_point[_mesh_idx][0], 
	//	-meshes_rotation_point[_mesh_idx][1], -meshes_rotation_point[_mesh_idx][2]);
	//glRotatef(angley, 1, 0, 0);
	//glRotatef(anglex, 0, 1, 0);
	//glTranslatef(meshes_rotation_point[_mesh_idx][0],
	//	meshes_rotation_point[_mesh_idx][1], meshes_rotation_point[_mesh_idx][2]);
	//glRotatef(180.f, 0, 1, 0);
	//glRotatef(90.f, 0, 0, 1);
	//glRotatef(180.f, 0, 0, 1);

	glBegin(GL_TRIANGLES);

	Mesh::FaceIter f_it;
	Mesh::FaceIter f_end = mesh2draw->faces_end();
	for (f_it = mesh2draw->faces_begin(); f_it != f_end; ++f_it)
	{
		Mesh::FaceVertexIter fv_it;
		for (fv_it = mesh2draw->fv_iter(*f_it); fv_it.is_valid(); ++fv_it)
		{
			//int v_idx = fv_it.handle().idx();

			Mesh::Point p = mesh2draw->point(*fv_it);
			//float point[3] {p[0] - meshes_center[_mesh_idx][0], 
			//	p[1] - meshes_center[_mesh_idx][1],
			//	p[2] - meshes_center[_mesh_idx][2]};

			float point[3] {p[0],
				p[1],
				p[2]};

			//Mesh::Normal n = mesh2draw->normal(*fv_it);
			//float normal[3] {n[0], n[1], n[2]};

			Mesh::Color c = mesh2draw->color(*fv_it);
			glColor3f(c[0], c[1], c[2]);

			//RowVectorXf n = normals2draw->row(v_idx);
			//float normal[3] {n(0), n(1), n(2)};

			//RowVectorXf c = colours2draw->row(v_idx);
			//glColor3f(c(0), c(1), c(2));

			//glNormal3fv(normal);
			glVertex3fv(point);
		}
	}
	glEnd();
}
//=============================================================================
void Viewer::zoom(GLfloat distance)
{
	translation[2] += distance;
}
//=============================================================================
void Viewer::calculateCenterPoint(int _mesh_idx)
{
	Mesh* curr_mesh = &meshes[_mesh_idx][0];

	GLfloat max_x = numeric_limits<float>::min(),
		max_y = numeric_limits<float>::min(),
		max_z = numeric_limits<float>::min();
	GLfloat min_x = numeric_limits<float>::max(),
		min_y = numeric_limits<float>::max(),
		min_z = numeric_limits<float>::max();
	Mesh::VertexIter v_it, v_end(curr_mesh->vertices_end());
	int i_v = 0;
	for (v_it = curr_mesh->vertices_begin(); v_it != v_end; v_it++)
	{
		Mesh::Point p = curr_mesh->point(*v_it);
		max_x = max(max_x, p[0]);
		max_y = max(max_y, p[1]);
		max_z = max(max_z, p[2]);
		min_x = min(min_x, p[0]);
		min_y = min(min_y, p[1]);
		min_z = min(min_z, p[2]);
	}

	meshes_center[_mesh_idx].resize(3);
	meshes_center[_mesh_idx][0] = min_x + (max_x - min_x) / 2;
	meshes_center[_mesh_idx][0] = min_y + (max_y - min_y) / 2;
	meshes_center[_mesh_idx][0] = min_z + (max_z - min_z) / 2;
}
//=============================================================================
void Viewer::initialize(int *argc, char **argv)
{
	string config_filename = argv[1];
	params.load(config_filename);

	loadMeshes();

	initGLUT(argc, argv);
	initGLUI();
}
//=============================================================================
/*
 * Reads mesh from file and computes the normals if not provided.
 * This mesh is loaded basicly to have faces
 */
void Viewer::loadMesh(string _mesh_filename, Mesh* _mesh)
{
	Mesh* curr_mesh = _mesh;

	readMesh(*curr_mesh, _mesh_filename.c_str(), ropt);

	// Add vertex normals
	//curr_mesh->request_vertex_normals();

	// Add face normals
	//curr_mesh->request_face_normals();

	////If the file did not provide vertex normals, then calculate them
	//if (curr_mesh->has_face_normals() && curr_mesh->has_vertex_normals())
	//{
	//	// let the mesh update the normals
	//	curr_mesh->update_normals();
	//}

	// Add vertex colors

	//Mesh::ConstVertexIter v_it;
	//Mesh::ConstVertexIter v_end(curr_mesh->vertices_end());
	//for (v_it = curr_mesh->vertices_begin(); v_it != v_end; ++v_it)
	//{
	//	curr_mesh->set_color(*v_it, MODEL_COLOR);
	//}
}
//=============================================================================
void Viewer::run()
{
	last_time = clock();

	glutMainLoop();
}
//=============================================================================
void Viewer::destroy()
{

}
//=============================================================================
void Viewer::loadMeshes()
{
	std::cout << "Loading meshes... ";

	ropt += OpenMesh::IO::Options::VertexNormal;
	ropt += OpenMesh::IO::Options::VertexColor;

	clock_t start;
	start = clock();

	meshes.resize(params.n_meshes);
	meshes_center.resize(params.n_meshes);
	for (int i = 0; i < params.n_meshes; i++)
	{
		meshes[i].resize(params.n_frames);
		for (int j = 0; j < params.n_frames; j++)
		{
			string mesh_filename = getMeshFilename(i, j);
			readMesh(meshes[i][j], mesh_filename, ropt);
		}

		calculateCenterPoint(i);
	}

	std::cout << (clock() - start) / (double)(CLOCKS_PER_SEC / 1000)
		<< " ms" << endl;
}
//=============================================================================
string Viewer::cvtIntToString(int _n, int _no_digits)
{
	int no_zeros = _no_digits - numDigits(_n);

	return string(no_zeros, '0').append(to_string(_n));
}
//=============================================================================
int Viewer::numDigits(int _number)
{
	int digits = 0;
	while (_number) {
		_number /= 10;
		digits++;
	}

	return digits;
}
//=============================================================================
MatrixXf Viewer::normals2colour(const MatrixXf &_normals)
{
	MatrixXf colours = _normals;
	colours += MatrixXf::Ones(colours.rows(), colours.cols());
	colours /= 2;

	return colours;
}
//=============================================================================
MatrixXf Viewer::normals2SHfunctions(const MatrixXf &_normals, int _order)
{
	MatrixXf h;
	int no_points = _normals.rows();

	VectorXf x = _normals.col(0);				// x
	VectorXf y = _normals.col(1);				// y 
	VectorXf z = _normals.col(2);				// z
	VectorXf x2 = x.cwiseProduct(x);			// x^2
	VectorXf y2 = y.cwiseProduct(y);			// y^2
	VectorXf z2 = z.cwiseProduct(z);			// z^2
	VectorXf xy = x.cwiseProduct(y);			// x * y
	VectorXf xz = x.cwiseProduct(z);			// x * z
	VectorXf yz = y.cwiseProduct(z);			// y * z
	VectorXf x2_y2 = x2 - y2;					// x^2 - y^2
	VectorXf ones = VectorXf::Ones(no_points);	// 1

	h = MatrixXf::Ones(no_points, (_order + 1) * (_order + 1));

	if (_order > 0)
		h.block(0, 1, no_points, 3) = _normals;	// x, y, z

	if (_order > 1)
	{
		h.col(4) = xy;				// x * y
		h.col(5) = xz;				// x * z
		h.col(6) = yz;				// y * z
		h.col(7) = x2_y2;			// x^2 - y^2
		h.col(8) = 3 * z2 - ones;	// 3 * z^2 - 1
	}

	if (_order > 2)
	{
		h.col(9) = (3 * x2 - y2).cwiseProduct(y);			// (3 * x^2 - y^2) * y 
		h.col(10) = xy.cwiseProduct(z);						// x * y * z
		h.col(11) = (5 * z2 - ones).cwiseProduct(y);		// (5 * z^2 - 1) * y
		h.col(12) = (5 * z2 - 3 * ones).cwiseProduct(z);	// (5 * z^2 - 3) * z
		h.col(13) = (5 * z2 - ones).cwiseProduct(x);		// (5 * z^2 - 1) * x
		h.col(14) = x2_y2.cwiseProduct(z);					// (x^2 - y^2) * z
		h.col(15) = (x2 - 3 * y2).cwiseProduct(x);			// (x^2 - 3 * y^2) * x
	}

	if (_order > 3)
	{
		h.col(16) = x2_y2.cwiseProduct(xy);							// (x^2 - y^2) * x * y
		h.col(17) = (3 * x2 - y2).cwiseProduct(yz);					// (3 * x^2 - y^2) * yz
		h.col(18) = (7 * z2 - ones).cwiseProduct(xy);				// (7 * z^2 - 1) * x * y
		h.col(19) = (7 * z2 - 3 * ones).cwiseProduct(yz);			// (7 * z^2 - 3) * y * z
		h.col(20) = 3 * ones - 30 * z2 + 35 * z2.cwiseProduct(z2);	// 3 - 30 * z^2 + 35 * z^4
		h.col(21) = (7 * z2 - 3 * ones).cwiseProduct(xz);			// (7 * z^2 - 3) * x * z
		h.col(22) = (7 * z2 - ones).cwiseProduct(x2_y2);			// (7 * z^2 - 1) * (x^2 - y^2)
		h.col(23) = (x2 - 3 * y2).cwiseProduct(xz);					// (x^2 - 3 * y^2) * x * z
		h.col(24) = (x2 - 3 * y2).cwiseProduct(x2)					// (x^2 - 3 * y^2) * x^2 - (3 * x^2 - y^2) * y^2 
			- (3 * x2 - y2).cwiseProduct(y2);
	}

	return h;
}
//=============================================================================
string Viewer::getMeshFilename(int _mesh_idx, int _frame_idx)
{
	string filename = params.mesh_prefix[_mesh_idx];
	int idx = _frame_idx + params.mesh_first_idx[_mesh_idx];
	filename.append(cvtIntToString(idx, params.mesh_n_digits[_mesh_idx]));
	filename.append(params.mesh_suffix[_mesh_idx]);

	return filename;
}
//=============================================================================
