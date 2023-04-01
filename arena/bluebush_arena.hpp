/*
	Bluebush 3D API 'Fuel' 1.9 Arena PRODUCTION
	Property of Cameron King 2022

	Author: Cameron King
	Date: 25th October 2022

*/

#include <iostream>
#include <fstream>
//#include <string>
#define NO_G3D_ALLOCATOR 1
#include <SIMDString.h>
//#include <cstring>
#include <numeric>
#include <vector>
#include <map>
#include <math.h>
#include <mutex>
#include <chrono>
#include <thread>

#define GLEW_STATIC 1
#include <gl/glew.h>
#include <Fl/GLU.h>

#define WIN32
#include <FL/Fl.H>
#include <FL/platform.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>

using namespace std;

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

struct marker_type
{
	int index = -1;
	bool isStatic = true;
	short index_type = -1;
};

struct point
{
	point() {};
	float x = 0;
	float y = 0;
	point(float _x, float _y) : x(_x), y(_y) {}
};

struct ipoint
{
	ipoint() {}
	int x = 0;
	int y = 0;
	ipoint(int _x, int _y) : x(_x), y(_y) {}
};

struct coords3d
{
	coords3d() {}
	float x = 0;
	float y = 0;
	float z = 0;
	coords3d(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct quat
{
	quat() {}
	float x = 0;
	float y = 0;
	float z = 0;
	float s = 0;
	quat(float _x, float _y, float _z, float _s) : x(_x), y(_y), z(_z), s(_s) {}
};

struct obj
{
	int startimage = 0;
	int count = 0;
};

struct tri
{
	coords3d a{};
	coords3d b{};
	coords3d c{};
};

struct f_result
{
	coords3d c = { 0.0f, 0.0f, 0.0f };
	int i = 0;
	float f = 0.0f;
	bool success = false;
};

struct araQuad
{
	string tex_name = "";
	coords3d v[4]; // Vertices
	coords3d n[4]; // Normals
	coords3d quad_normal = { 0.0f, 0.0f, 0.0f };
	ipoint t[4];	// Texture coords
	int quad_handle = 0; // "image handle" returned by bluebush.
};

struct araMaterial
{
	bool faceted = false;
	float alpha = 1.0f;
	float fresnel = 0.0f;
	float envmap = 0.0f;
	string autotex_image = "";
	ipoint autotex_offset = { 0, 0 };
	float autotex_zoom = 1.0f;
	float autotex_mixratio = 0.0f;
	string multitex_image = "";
	float multitex_zoom = 1.0f;
	float multitex_mixratio = 0.0f;
	bool reverse_winding = false;
};

struct araFormat
{
	araQuad q;
	int quad_count = 0;
	araMaterial m;
};

coords3d operator+(const coords3d a, const coords3d b) // Addition of coords3d pair
{
	coords3d r;

	r.x = a.x + b.x;
	r.y = a.y + b.y;
	r.z = a.z + b.z;

	return r;
}

bool global_suppressImages = false;
bool orthoMode = false;

GLuint v, f, program;

GLuint att_PARASCAX_id;

GLuint lightmixLocation;

GLuint PAlocation;
GLuint RAlocation;
GLuint SXlocation;
GLuint TDlocation;
GLuint MXlocation;
GLuint TXlocation;

GLuint texlocation;
GLuint envlocation;
GLuint camlocation;

GLuint atlastextureID;
GLuint FBOtextureID;
GLuint ENVtextureID;

int charpressd;
unsigned int keyboard_buffer = -1;
float mousewheel_val = 0;

int atlas_x[1024];	// Max 1024 images in atlas
int atlas_y[1024];
int atlas_xsize[1024];
int atlas_ysize[1024];

/*
const int main_back_submit = 24000;
const int ZC_back_submit = 16426;
const int HUD_back_submit = 4970; // 1749

const int main_geometry_limit = 100840; // We will automatically calculate all limits from this now. 5500 4300
const int ZC_geometry_limit = 16430; // 701
const int HUD_geometry_limit = 4980;
*/

const int main_back_submit = 1;
const int ZC_back_submit = 1;
const int HUD_back_submit = 300; // 1749

const int main_geometry_limit = 100000; // We will automatically calculate all limits from this now. 5500 4300
const int ZC_geometry_limit = 100000; // 701
const int HUD_geometry_limit = 600;

float PARASCAX_main_mini[main_back_submit * 16];
float PARASCAX_ZC_mini[ZC_back_submit * 16];
float PARASCAX_HUD_mini[HUD_back_submit * 16];

const int total_geometry_limit = main_geometry_limit + ZC_geometry_limit + HUD_geometry_limit;

const int PARASCAX_limit = (total_geometry_limit * 4) * 24; // 4 verts x 24 comp PARASCAX (PA-RA-SX-TD-MX-TX). 4 * 6 = 24

const int vertnorm_array_limit = (total_geometry_limit * 4) * 3; // 4 verts, 3 XYZ
const int texcolor_array_limit = (total_geometry_limit * 4) * 4; // 4 verts, 4 RGBA

const int ZC_vertnorm_offset = main_geometry_limit * (4 * 3); // 4 verts, 3 XYZ
const int ZC_texcolor_offset = main_geometry_limit * (4 * 4); // 4 verts, 4 RGBA

const int HUD_vertnorm_offset = (main_geometry_limit + ZC_geometry_limit) * (4 * 3); // 4 verts, 3 XYZ
const int HUD_texcolor_offset = (main_geometry_limit + ZC_geometry_limit) * (4 * 4); // 4 verts, 4 RGBA

const int draw_main_lower = 0;
const int draw_main_upper = main_geometry_limit * 4; // 4 verts

const int draw_ZC_lower = draw_main_upper;
const int draw_ZC_upper = ZC_geometry_limit * 4; // 4 verts

const int draw_HUD_lower = draw_ZC_lower + draw_ZC_upper;
const int draw_HUD_upper = (HUD_geometry_limit * 4); // 4 verts

int image_tex_i[main_geometry_limit];
int ZCimage_tex_i[ZC_geometry_limit];
int HUDimage_tex_i[HUD_geometry_limit];

point image_size[main_geometry_limit]; // get image dimensions
point HUDimage_size[HUD_geometry_limit];

obj object_i[main_geometry_limit]; // Each object can contain as little as one image or a max of all allocated images, aka, main_geometry_limit.
obj ZCobject_i[ZC_geometry_limit];
obj HUDobject_i[HUD_geometry_limit];

float att_PARASCAX[PARASCAX_limit]; // 4 comp x 3 (because we added them together).

float vertex_array[vertnorm_array_limit]; // 3 comp per vertex (3 vertices 1 1D displacementMap coords)
float normal_array[vertnorm_array_limit]; // 3 comp per normal
float texcoord_array[texcolor_array_limit]; // 4 comp (2 texcoord 1 fresnel ratio 1 cube mapping )
float color_array[texcolor_array_limit]; // 4 comp

coords3d camera, look, cameraZC, lookZC;

bool wireframe_mode = false;

class bluebush : public Fl_Gl_Window
{
private:

	string failedtofind;

	bool updateVBO;
	bool updateShaders;

	multimap <string, int> atlas_map;

	int object_index;
	int ZCobject_index;
	int HUDobject_index;

	int image_index;
	int vert_i;
	int tex_i;
	int color_i;

	bool dyn_mode;
	int dyn_image_index;
	int dyn_ZCimage_index;
	int dyn_HUDimage_index;

	int dyn_vert_i;
	int dyn_tex_i;
	int dyn_color_i;

	int dyn_Zvert_i;
	int dyn_Ztex_i;
	int dyn_Zcolor_i;

	int dyn_Hvert_i;
	int dyn_Htex_i;
	int dyn_Hcolor_i;

	int ZCimage_index;
	int Zvert_i;
	int Ztex_i;
	int Zcolor_i;

	int HUDimage_index;
	int Hvert_i;
	int Htex_i;
	int Hcolor_i;

	GLuint vertex_id;
	GLuint normal_id;
	GLuint texcoord_id;
	GLuint color_id;

	float camUniform[3] = { 0.0f, 0.0f, 0.0f };

	coords3d lightPos = { 0.0f, 0.0f, 0.0f };

	int xres;
	int yres;
	float M_PI;

	GLdouble near_posX, near_posY, near_posZ;
	GLdouble far_posX, far_posY, far_posZ;
	GLdouble far_posX_zc, far_posY_zc, far_posZ_zc;

	mutex image_mutex;
	mutex object_mutex;

	int displacementA = 0;
	int displacementB = 0;
	int displacementC = 0;
	int displacementD = 0;

	coords3d WTS_in[16];
	coords3d WTS_out[16];

	// This is a hack :-( to package MultiTex mix GLSL attribute with DisplacementMix mix attribute in a 1D single-component array.
	float global_MTmix_main = 0.0f;
	float global_displacementMix_main = 0.0f;
	float global_MTmix_ZC = 0.0f;
	float global_displacementMix_ZC = 0.0f;
	float global_MTmix_HUD = 0.0f;
	float global_displacementMix_HUD = 0.0f;

	int global_key = -1;

	void printShaderInfoLog(GLuint obj)
	{
		int infologLength = 0;
		int charsWritten = 0;
		char* infoLog;

		glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

		if (infologLength > 0)
		{
			infoLog = (char*)malloc(infologLength);
			glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
			printf("%s\n", infoLog);
			free(infoLog);
		}
	}

	void printProgramInfoLog(GLuint obj)
	{
		int infologLength = 0;
		int charsWritten = 0;
		char* infoLog;

		glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

		if (infologLength > 0)
		{
			infoLog = (char*)malloc(infologLength);
			glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
			printf("%s\n", infoLog);
			free(infoLog);
		}
	}

	void checkLimits(string filename, int index_type, int manifold_count) // To check if loading an object will violate the limits.
	{
		if (index_type == 1) // main
		{
			marker_type mt = getMarker_main();

			if (mt.isStatic == true)
			{
				if ((mt.index + manifold_count) > (main_geometry_limit - main_back_submit)) // +1 just as a buffer
				{
					cout << "LoadObject '" << filename << "' will exceed static allocation by " << (mt.index + manifold_count + 1) - (main_geometry_limit - main_back_submit) << " manifolds!\n";
					shutdown();
				}
			}
			else if (mt.isStatic == false)
			{
				if ((mt.index + manifold_count) > main_geometry_limit) // +1 just as a buffer
				{
					cout << "LoadObject '" << filename << "' will exceed dynamic allocation by " << (mt.index + manifold_count + 1) - main_geometry_limit << " manifolds!\n";
					shutdown();
				}
			}
		}
		else if (index_type == 2) // ZC
		{
			marker_type mt = getMarker_ZC();

			if (mt.isStatic == true)
			{
				if ((mt.index + manifold_count) > (ZC_geometry_limit - ZC_back_submit)) // +1 just as a buffer
				{
					cout << "LoadZCObject '" << filename << "' will exceed static allocation by " << (mt.index + manifold_count + 1) - (ZC_geometry_limit - ZC_back_submit) << " manifolds!\n";
					shutdown();
				}
			}
			else if (mt.isStatic == false)
			{
				if ((mt.index + manifold_count) > ZC_geometry_limit) // +1 just as a buffer
				{
					cout << "LoadZCObject '" << filename << "' will exceed dynamic allocation by " << (mt.index + manifold_count + 1) - ZC_geometry_limit << " manifolds!\n";
					shutdown();
				}
			}
		}
		else if (index_type == 3) // HUD
		{
			marker_type mt = getMarker_HUD();

			if (mt.isStatic == true)
			{
				if ((mt.index + manifold_count) > (HUD_geometry_limit - HUD_back_submit)) // +1 just as a buffer
				{
					cout << "LoadHUDObject '" << filename << "' will exceed static allocation by " << (mt.index + manifold_count + 1) - (HUD_geometry_limit - HUD_back_submit) << " manifolds!\n";
					shutdown();
				}
			}
			else if (mt.isStatic == false)
			{
				if ((mt.index + manifold_count) > HUD_geometry_limit) // +1 just as a buffer
				{
					cout << "LoadHUDObject '" << filename << "' will exceed dynamic allocation by " << (mt.index + manifold_count + 1) - HUD_geometry_limit << " manifolds!\n";
					shutdown();
				}
			}
		}
	}

	bool loadatlas()
	{
		// MD5 stuff here.

		string line_string;

		int atlas_inc = 0;
		string imagename;

		ifstream manfile("assets/atlas.manifest");

		if (manfile.is_open())
		{
			while (!manfile.eof())
			{
				getline(manfile, imagename); // name

				getline(manfile, line_string); // x
				atlas_x[atlas_inc] = atoi(line_string.c_str());

				getline(manfile, line_string); // y
				atlas_y[atlas_inc] = atoi(line_string.c_str());

				getline(manfile, line_string); // xsize
				atlas_xsize[atlas_inc] = atoi(line_string.c_str());

				getline(manfile, line_string); // ysize
				atlas_ysize[atlas_inc] = atoi(line_string.c_str());

				atlas_map.insert(pair<string, int>(imagename, atlas_inc));
				atlas_inc++;
			}
			manfile.close();
		}
		else
		{
			cout << "Unable to open manifest";
			shutdown();
		}

		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &atlastextureID);

		vector <unsigned char> uncomp_image, buffer;
		unsigned long width = 0;
		unsigned long height = 0;

		loadPNG(buffer, "assets/atlas.dat");
		int error = decodePNG(uncomp_image, width, height, buffer.empty() ? 0 : &buffer[0], (unsigned long)buffer.size(), true);

		glBindTexture(GL_TEXTURE_2D, atlastextureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // GL_LINEAR
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // GL_LINEAR

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2048, 2048, 0, GL_RGBA, GL_UNSIGNED_BYTE, &uncomp_image[0]);

		glEnable(GL_TEXTURE_2D);	// using GL_COMPRESSED_RGBA_S3TC_DXT5

		return true;
	}

	void setupshaders()
	{
		// The main shaders.

		GLint MaxVertexAttribs;

		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &MaxVertexAttribs);

			cout << "Max attribs = " << MaxVertexAttribs << "\n";

		ifstream vfile("assets\/arena_vertex.txt");
		string vfile_contents;
		string vline;
		while (getline(vfile, vline))
			vfile_contents += vline + '\n';
		vfile.close();

		const char* vs = vfile_contents.c_str();
		
		ifstream ffile("assets\/arena_fragment.txt");
		string ffile_contents;
		string fline;
		while (getline(ffile, fline))
			ffile_contents += fline + '\n';
		ffile.close();

		const char* fs = ffile_contents.c_str();

		v = glCreateShader(GL_VERTEX_SHADER);
		f = glCreateShader(GL_FRAGMENT_SHADER);

		glShaderSource(v, 1, &vs, NULL);
		glShaderSource(f, 1, &fs, NULL);

		glCompileShader(v);
		glCompileShader(f);

		printShaderInfoLog(v);
		printShaderInfoLog(f);

		program = glCreateProgram();

		glAttachShader(program, v);
		glAttachShader(program, f);

		glBindAttribLocation(program, 9, "PA"); // Keep our indices from colliding with 2 and 3 (nor
		glBindAttribLocation(program, 10, "RA"); // gl_Normal collides
		glBindAttribLocation(program, 11, "SX"); // gl_Color collides
		glBindAttribLocation(program, 12, "TD");
		glBindAttribLocation(program, 13, "MX");
		glBindAttribLocation(program, 14, "TX");

		glLinkProgram(program);
		glUseProgram(program);

		printProgramInfoLog(program);

		lightmixLocation = glGetUniformLocation(program, "lightmix");
		glUniform1f(lightmixLocation, 1.0f); // Initial light mix is full.

		PAlocation = glGetAttribLocation(program, "PA");
		RAlocation = glGetAttribLocation(program, "RA");
		SXlocation = glGetAttribLocation(program, "SX");
		TDlocation = glGetAttribLocation(program, "TD");
		MXlocation = glGetAttribLocation(program, "MX");
		TXlocation = glGetAttribLocation(program, "TX");

		glActiveTexture(GL_TEXTURE0);
		texlocation = glGetUniformLocation(program, "tex0");
		glUniform1i(texlocation, 0);

		glActiveTexture(GL_TEXTURE2);
		envlocation = glGetUniformLocation(program, "env0");
		glUniform1i(envlocation, 2);
	}

	void setuplight()
	{
		glEnable(GL_LIGHT0);
		glEnable(GL_NORMALIZE);

		// Light model parameters:

		GLfloat lmKa[] = { 0.0, 0.0, 0.0, 0.0 };

		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmKa);
		glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0);
		glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 0.0);

		// Spotlight Attenuation

		GLfloat spot_direction[] = { 0.0, 0.0, 1.0 };
		GLint spot_exponent = 30;
		GLint spot_cutoff = 180;

		glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);
		glLighti(GL_LIGHT0, GL_SPOT_EXPONENT, spot_exponent);
		glLighti(GL_LIGHT0, GL_SPOT_CUTOFF, spot_cutoff);

		GLfloat Kc = 1.0;
		GLfloat Kl = 0.0;
		GLfloat Kq = 0.0;

		glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, Kc);
		glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, Kl);
		glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, Kq);

		// Lighting parameters:

		GLfloat light_pos[] = { lightPos.x, lightPos.y, lightPos.z, 1.0f };
		GLfloat light_Ka[] = { 0.1f, 0.1f, 0.1f, 1.0f };
		GLfloat light_Kd[] = { 0.15f, 0.15f, 0.15f, 1.0f }; // was 0.7
		GLfloat light_Ks[] = { 0.9f, 0.9f, 0.9f, 1.0f };

		glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_Ka);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_Kd);
		glLightfv(GL_LIGHT0, GL_SPECULAR, light_Ks);

		// Material parameters:

		GLfloat material_Ka[] = { 0.25f, 0.26f, 0.2f, 1.0f };
		GLfloat material_Kd[] = { 0.4f, 0.3f, 0.3f, 1.0f };
		GLfloat material_Ks[] = { 0.7f, 0.65f, 0.6f, 1.0f };
		GLfloat material_Ke[] = { 1.0f, 0.92f, 0.9f, 0.0f };
		GLfloat material_Se = 50.0f; // 10.0f

		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_Ka);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_Kd);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_Ks);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_Ke);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material_Se);
	}

	void setupvbo()
	{
		// main geometry initialization

		glGenBuffers(1, &vertex_id);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_id);
		memset(vertex_array, 0, sizeof(vertex_array));
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_array), vertex_array, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &normal_id);
		glBindBuffer(GL_ARRAY_BUFFER, normal_id);
		memset(normal_array, 0, sizeof(normal_array));
		glBufferData(GL_ARRAY_BUFFER, sizeof(normal_array), normal_array, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &texcoord_id);
		glBindBuffer(GL_ARRAY_BUFFER, texcoord_id);
		memset(texcoord_array, 0, sizeof(texcoord_array));
		glBufferData(GL_ARRAY_BUFFER, sizeof(texcoord_array), texcoord_array, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &color_id);
		glBindBuffer(GL_ARRAY_BUFFER, color_id);
		memset(color_array, 1, sizeof(color_array));
		glBufferData(GL_ARRAY_BUFFER, sizeof(color_array), color_array, GL_DYNAMIC_DRAW);

		// shader attributes

		glGenBuffers(1, &att_PARASCAX_id);
		glBindBuffer(GL_ARRAY_BUFFER, att_PARASCAX_id);
		memset(att_PARASCAX, 0, sizeof(att_PARASCAX));
		glBufferData(GL_ARRAY_BUFFER, sizeof(att_PARASCAX), att_PARASCAX, GL_DYNAMIC_DRAW);

		// Purposely leave att_PARASCAX bound.
	}

	void setupenvmap()
	{
		glActiveTexture(GL_TEXTURE2);
		glGenTextures(1, &ENVtextureID);

		glBindTexture(GL_TEXTURE_CUBE_MAP, ENVtextureID);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		vector <unsigned char> cube_image, buffer;
		unsigned long width = 0;
		unsigned long height = 0;

		cube_image.clear();
		buffer.clear();

		loadPNG(buffer, "assets/env_p_x.dat");

		int error = decodePNG(cube_image, width, height, buffer.empty() ? 0 : &buffer[0], (unsigned long)buffer.size(), true);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &cube_image[0]);

		cube_image.clear();
		buffer.clear();
		loadPNG(buffer, "assets/env_n_x.dat");
		error = decodePNG(cube_image, width, height, buffer.empty() ? 0 : &buffer[0], (unsigned long)buffer.size(), true);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &cube_image[0]);

		cube_image.clear();
		buffer.clear();
		loadPNG(buffer, "assets/env_p_y.dat");
		error = decodePNG(cube_image, width, height, buffer.empty() ? 0 : &buffer[0], (unsigned long)buffer.size(), true);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &cube_image[0]);

		cube_image.clear();
		buffer.clear();
		loadPNG(buffer, "assets/env_n_y.dat");
		error = decodePNG(cube_image, width, height, buffer.empty() ? 0 : &buffer[0], (unsigned long)buffer.size(), true);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &cube_image[0]);

		cube_image.clear();
		buffer.clear();
		loadPNG(buffer, "assets/env_p_z.dat");
		error = decodePNG(cube_image, width, height, buffer.empty() ? 0 : &buffer[0], (unsigned long)buffer.size(), true);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &cube_image[0]);

		cube_image.clear();
		buffer.clear();
		loadPNG(buffer, "assets/env_n_z.dat");
		error = decodePNG(cube_image, width, height, buffer.empty() ? 0 : &buffer[0], (unsigned long)buffer.size(), true);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &cube_image[0]);
	}

	f_result modifyWorldToScreen(bool create, bool remove, bool view, int handle, coords3d worldpos)
	{
		f_result r;
		r.success = false;

		static int WTSindex = 0;
		static int WTSrecycle[16];
		static int WTSstack = 0;

		if (create == true)
		{
			if (WTSstack == 0 && WTSindex >= 16) // If the recycle bin is empty and the index is at maximum, signal the failure.
			{
				r.success = false;
			}
			else if (WTSstack == 0 && WTSindex < 16) // if recycle bin is empty, but we have space, provide the handle.
			{
				WTSindex++;
				r.i = WTSindex;
				r.success = true;
			}
			else if (WTSstack > 0) // We can use a recycled index.
			{
				r.i = WTSrecycle[WTSstack - 1]; // WE MAYBE NEED TO BE CAREFUL WHERE WE INTEND INDEXES AND HANDLES TO APPEAR.
				r.success = true;
				WTSstack--;
			}
		}

		if (remove == true)
		{
			WTS_in[handle - 1] = { 0.0f, 0.0f, 0.0f };
			if (WTSstack > 15)
			{
				cout << "!! Over-removal of WTS handles detected !!\n";
				r.success = false;
			}
			else
			{
				WTSrecycle[WTSstack] = handle;
				WTSstack++;
				r.success = true;
			}
		}

		if (view == true)
		{
			WTS_in[handle - 1] = worldpos;
			r.c = WTS_out[handle - 1];
			r.success = true;
		}

		return r;
	}

	void opengl_core()
	{
		if (updateVBO == true) // If this flag is true, reload the VBO with the current state of the entire vertex_array.
		{
			updateVBO = false;

			glBindBuffer(GL_ARRAY_BUFFER, vertex_id);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_array), vertex_array, GL_STREAM_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, normal_id);
			glBufferData(GL_ARRAY_BUFFER, sizeof(normal_array), normal_array, GL_STREAM_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, texcoord_id);
			glBufferData(GL_ARRAY_BUFFER, sizeof(texcoord_array), texcoord_array, GL_STREAM_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, color_id);
			glBufferData(GL_ARRAY_BUFFER, sizeof(color_array), color_array, GL_STREAM_DRAW);

			// Update vertex attributes for shaders

			glBindBuffer(GL_ARRAY_BUFFER, att_PARASCAX_id);

			glBufferData(GL_ARRAY_BUFFER, sizeof(att_PARASCAX), NULL, GL_DYNAMIC_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(att_PARASCAX), att_PARASCAX);
		}

		if (updateShaders == true)
		{
			// Update vertex attributes for shaders ONLY

			glBindBuffer(GL_ARRAY_BUFFER, att_PARASCAX_id);

			glBufferData(GL_ARRAY_BUFFER, sizeof(att_PARASCAX), NULL, GL_DYNAMIC_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(att_PARASCAX), att_PARASCAX);

			updateShaders = false;
		}
		else
		{
			// Update vertex attributes for shaders

			int XX_main = (main_geometry_limit - main_back_submit) * (16 * sizeof(float));
			int XX_ZC = ((main_geometry_limit + ZC_geometry_limit) - ZC_back_submit) * (16 * sizeof(float));
			int XX_HUD = ((main_geometry_limit + ZC_geometry_limit + HUD_geometry_limit) - HUD_back_submit) * (16 * sizeof(float));
			int XX_portion = (main_geometry_limit + ZC_geometry_limit + HUD_geometry_limit) * (16 * sizeof(float));

			int mem_main = ((main_geometry_limit - main_back_submit) * 16);
			int mem_ZC = (((main_geometry_limit + ZC_geometry_limit) - ZC_back_submit) * 16);
			int mem_HUD = (((main_geometry_limit + ZC_geometry_limit + HUD_geometry_limit) - HUD_back_submit) * 16);
			int mem_portion = ((main_geometry_limit + ZC_geometry_limit + HUD_geometry_limit) * 16);

			glBindBuffer(GL_ARRAY_BUFFER, att_PARASCAX_id);

			memcpy(PARASCAX_main_mini, &att_PARASCAX[mem_main], sizeof(PARASCAX_main_mini));
			memcpy(PARASCAX_ZC_mini, &att_PARASCAX[mem_ZC], sizeof(PARASCAX_ZC_mini));
			memcpy(PARASCAX_HUD_mini, &att_PARASCAX[mem_HUD], sizeof(PARASCAX_HUD_mini));

			glBufferSubData(GL_ARRAY_BUFFER, XX_main, sizeof(PARASCAX_main_mini), PARASCAX_main_mini);	// PA Main
			glBufferSubData(GL_ARRAY_BUFFER, XX_ZC, sizeof(PARASCAX_ZC_mini), PARASCAX_ZC_mini);		// PA ZC
			glBufferSubData(GL_ARRAY_BUFFER, XX_HUD, sizeof(PARASCAX_HUD_mini), PARASCAX_HUD_mini);		// PA HUD

			memcpy(PARASCAX_main_mini, &att_PARASCAX[mem_portion + mem_main], sizeof(PARASCAX_main_mini));
			memcpy(PARASCAX_ZC_mini, &att_PARASCAX[mem_portion + mem_ZC], sizeof(PARASCAX_ZC_mini));
			memcpy(PARASCAX_HUD_mini, &att_PARASCAX[mem_portion + mem_HUD], sizeof(PARASCAX_HUD_mini));

			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_main, sizeof(PARASCAX_main_mini), PARASCAX_main_mini);		// RA Main
			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_ZC, sizeof(PARASCAX_ZC_mini), PARASCAX_ZC_mini);			// RA ZC
			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_HUD, sizeof(PARASCAX_HUD_mini), PARASCAX_HUD_mini);		// RA HUD

			memcpy(PARASCAX_main_mini, &att_PARASCAX[mem_portion + mem_portion + mem_main], sizeof(PARASCAX_main_mini));
			memcpy(PARASCAX_ZC_mini, &att_PARASCAX[mem_portion + mem_portion + mem_ZC], sizeof(PARASCAX_ZC_mini));
			memcpy(PARASCAX_HUD_mini, &att_PARASCAX[mem_portion + mem_portion + mem_HUD], sizeof(PARASCAX_HUD_mini));

			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_main, sizeof(PARASCAX_main_mini), PARASCAX_main_mini);	// SX Main
			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_ZC, sizeof(PARASCAX_ZC_mini), PARASCAX_ZC_mini);			// SX ZC
			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_HUD, sizeof(PARASCAX_HUD_mini), PARASCAX_HUD_mini);		// SX HUD

			memcpy(PARASCAX_main_mini, &att_PARASCAX[mem_portion + mem_portion + mem_portion + mem_main], sizeof(PARASCAX_main_mini));
			memcpy(PARASCAX_ZC_mini, &att_PARASCAX[mem_portion + mem_portion + mem_portion + mem_ZC], sizeof(PARASCAX_ZC_mini));
			memcpy(PARASCAX_HUD_mini, &att_PARASCAX[mem_portion + mem_portion + mem_portion + mem_HUD], sizeof(PARASCAX_HUD_mini));

			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_portion + XX_main, sizeof(PARASCAX_main_mini), PARASCAX_main_mini);	// TD Main
			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_portion + XX_ZC, sizeof(PARASCAX_ZC_mini), PARASCAX_ZC_mini);			// TD ZC
			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_portion + XX_HUD, sizeof(PARASCAX_HUD_mini), PARASCAX_HUD_mini);		// TD HUD

			memcpy(PARASCAX_main_mini, &att_PARASCAX[mem_portion + mem_portion + mem_portion + mem_portion + mem_main], sizeof(PARASCAX_main_mini));
			memcpy(PARASCAX_ZC_mini, &att_PARASCAX[mem_portion + mem_portion + mem_portion + mem_portion + mem_ZC], sizeof(PARASCAX_ZC_mini));
			memcpy(PARASCAX_HUD_mini, &att_PARASCAX[mem_portion + mem_portion + mem_portion + mem_portion + mem_HUD], sizeof(PARASCAX_HUD_mini));

			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_portion + XX_portion + XX_main, sizeof(PARASCAX_main_mini), PARASCAX_main_mini);	// MX Main
			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_portion + XX_portion + XX_ZC, sizeof(PARASCAX_ZC_mini), PARASCAX_ZC_mini);		// MX ZC
			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_portion + XX_portion + XX_HUD, sizeof(PARASCAX_HUD_mini), PARASCAX_HUD_mini);		// MX HUD

			memcpy(PARASCAX_main_mini, &att_PARASCAX[mem_portion + mem_portion + mem_portion + mem_portion + mem_portion + mem_main], sizeof(PARASCAX_main_mini));
			memcpy(PARASCAX_ZC_mini, &att_PARASCAX[mem_portion + mem_portion + mem_portion + mem_portion + mem_portion + mem_ZC], sizeof(PARASCAX_ZC_mini));
			memcpy(PARASCAX_HUD_mini, &att_PARASCAX[mem_portion + mem_portion + mem_portion + mem_portion + mem_portion + mem_HUD], sizeof(PARASCAX_HUD_mini));

			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_portion + XX_portion + XX_portion + XX_main, sizeof(PARASCAX_main_mini), PARASCAX_main_mini);	// TX Main
			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_portion + XX_portion + XX_portion + XX_ZC, sizeof(PARASCAX_ZC_mini), PARASCAX_ZC_mini);		// TX ZC
			glBufferSubData(GL_ARRAY_BUFFER, XX_portion + XX_portion + XX_portion + XX_portion + XX_portion + XX_HUD, sizeof(PARASCAX_HUD_mini), PARASCAX_HUD_mini);	// TX HUD
		}
	}

	void render(coords3d camera, coords3d look, coords3d cameraZC, coords3d lookZC)
	{
		static float HUDnear = 160;
		static float HUDfar = -160;

		static coords3d cameraUP = { 0,0,0 };

		static GLint viewport[4];
		static GLdouble modelview[16];
		static GLdouble projection[16];

		static GLfloat winX, winY, winZ;

		// NOTE: For the Double-Agent project, the camera doesn't need to swing around the Z, so we just -1 for the left triangle portion.

		static coords3d cam_left = camera;
		cam_left.x = cam_left.x - 1.0f;

		tri t;

		t.a = camera;
		t.b = cam_left;
		t.c = look;

		cameraUP = { 0.0f, 0.0f, -1.0f };//calc_triangle_normal(t);

		if (wireframe_mode == true)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// main geometry 3D

		glUniform1f(lightmixLocation, 1.0f); // Turn on full light mixing.

		glEnable(GL_DEPTH_TEST);

		glViewport(0, 0, xres, yres);
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (orthoMode == true)
		{
			static float HUDnear2 = 2500;
			static float HUDfar2 = -2500;

			glUniform1f(lightmixLocation, 0.0f); // Turn off lights for HUD to avoid glare.

			glMatrixMode(GL_PROJECTION); // Ortho
			glLoadIdentity();
			glOrtho(0, xres, yres, 0, HUDnear2, HUDfar2);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		}
		else
		{
			glMatrixMode(GL_PROJECTION); // Perspective
			glLoadIdentity();
			gluPerspective(45.0, (GLfloat)xres / (GLfloat)yres, 0.1, 500.0); // 2020 settings were 0.01 -> 100.0
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		}

		gluLookAt(camera.x * 0.01, camera.y * 0.01, camera.z * 0.01, // normal camera
			look.x * 0.01, look.y * 0.01, look.z * 0.01,
			cameraUP.x, cameraUP.y, cameraUP.z);

		GLfloat light_pos[] = { lightPos.x, lightPos.y, lightPos.z, 1.0f }; // Re-define the light after the modelview transform, prevents it shifting with camera.
		glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

		// First 3D quads

		//glUseProgram(program); // Main shaders with lighting.

		//cout << "main:\n";
		//printProgramInfoLog(program);

		glDrawArrays(GL_QUADS, draw_main_lower, draw_main_upper); // 5500 * 4 = 22,000

		// resample the depth.

		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);		// gathered for GluProject
		glGetDoublev(GL_PROJECTION_MATRIX, projection);
		glGetIntegerv(GL_VIEWPORT, viewport);

		static GLdouble pro_x = 0;
		static GLdouble pro_y = 0;
		static GLdouble pro_z = 0;

		for (int i = 0; i < 16; i++)
		{
			gluProject(WTS_in[i].x * 0.01, WTS_in[i].y * 0.01, WTS_in[i].z * 0.01, modelview, projection, viewport, &pro_x, &pro_y, &pro_z);

			WTS_out[i] = { (float)pro_x, yres - (float)pro_y, 0.0f };
		}

		point mouse = { 0,0 };; // xymouse(); Apparently we just ignore the mouse for now.
		winX = mouse.x;
		winY = (float)viewport[3] - mouse.y;

		glReadPixels(winX, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
		gluUnProject(winX, winY, winZ, modelview, projection, viewport, &far_posX, &far_posY, &far_posZ); // Depth search

		// Z-cleared overlay geometry last 1500 quads in 3D

		glUniform1f(lightmixLocation, 1.0f); // Turn on full light mixing. (In case our Ortho above turned it off)

		glClear(GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0, (GLfloat)xres / (GLfloat)yres, 0.01, 1000.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		gluLookAt(cameraZC.x * 0.01, cameraZC.y * 0.01, cameraZC.z * 0.01,	// ZC (Z-cleared camera position)
			lookZC.x * 0.01, lookZC.y * 0.01, lookZC.z * 0.01,
			0.0, 0.0, -10.0);

		glLightfv(GL_LIGHT0, GL_POSITION, light_pos); // Re-define the light after the modelview transform by the camera

		glDrawArrays(GL_QUADS, draw_ZC_lower, draw_ZC_upper); // ZC 3D geometry

		// HUD geometry quads in 2D

		glUniform1f(lightmixLocation, 0.0f); // Turn off lights for HUD to avoid glare.

		glClear(GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, xres, yres, 0, HUDnear, HUDfar);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDrawArrays(GL_QUADS, draw_HUD_lower, draw_HUD_upper); // 2,000 vertices (actually, 1996 (-4) (now 499 quads) we steal a HUD quad for the FBO to display! ;-) ) 

		mousewheel_val = 0;	// avoid stale mousewheel value
	}

public:

	bluebush(int X, int Y, int W, int H, const char* L = 0) : Fl_Gl_Window(X, Y, W, H, L)
	{
		//global_key = -1;

		xres = W;
		yres = H;
		updateVBO = false;
		updateShaders = false;

		object_index = 0;
		ZCobject_index = 0;
		HUDobject_index = 0;

		image_index = 0;
		ZCimage_index = 0;
		HUDimage_index = 0;

		dyn_mode = false;
		dyn_image_index = (main_geometry_limit - main_back_submit);
		dyn_ZCimage_index = (ZC_geometry_limit - ZC_back_submit);
		dyn_HUDimage_index = (HUD_geometry_limit - HUD_back_submit);

		dyn_vert_i = (main_geometry_limit - main_back_submit) * 12;
		dyn_tex_i = (main_geometry_limit - main_back_submit) * 16;
		dyn_color_i = (main_geometry_limit - main_back_submit) * 16;

		vert_i = 0;
		tex_i = 0;
		color_i = 0;

		dyn_Zvert_i = (ZC_geometry_limit - ZC_back_submit) * 12;
		dyn_Ztex_i = (ZC_geometry_limit - ZC_back_submit) * 16;
		dyn_Zcolor_i = (ZC_geometry_limit - ZC_back_submit) * 16;

		Zvert_i = 0;
		Ztex_i = 0;
		Zcolor_i = 0;

		dyn_Hvert_i = (HUD_geometry_limit - HUD_back_submit) * 12;
		dyn_Htex_i = (HUD_geometry_limit - HUD_back_submit) * 16;
		dyn_Hcolor_i = (HUD_geometry_limit - HUD_back_submit) * 16;

		Hvert_i = 0;
		Htex_i = 0;
		Hcolor_i = 0;

		vertex_id = 0;
		texcoord_id = 0;
		color_id = 0;

		charpressd = -1;

		memset(WTS_in, 0.0f, sizeof(WTS_in));
		memset(WTS_out, 0.0f, sizeof(WTS_out));

		M_PI = 3.1415;

		camera = { 0,200,-200 }; // Sensible camera default.
		look = { 0,0,0 };
		cameraZC = { 0,200,-200 }; // Sensible camera default.
		lookZC = { 0,0,0 };
	}

	int getKey()
	{
		return global_key;
	}

	void setOrthoMode(bool state)
	{
		orthoMode = state;
	}

	void reloadShaders( const char* vert_string, const char* frag_string )
	{
		glDetachShader(program, v);
		glDetachShader(program, f);
		glDeleteShader(v);
		glDeleteShader(f);
		glDeleteProgram(program);

		v = glCreateShader(GL_VERTEX_SHADER);
		f = glCreateShader(GL_FRAGMENT_SHADER);

		glShaderSource(v, 1, &vert_string, NULL);
		glShaderSource(f, 1, &frag_string, NULL);

		glCompileShader(v);
		glCompileShader(f);

		printShaderInfoLog(v);
		printShaderInfoLog(f);

		program = glCreateProgram();

		glAttachShader(program, v);
		glAttachShader(program, f);

		glBindAttribLocation(program, 9, "PA");
		glBindAttribLocation(program, 10, "RA");
		glBindAttribLocation(program, 11, "SX");
		glBindAttribLocation(program, 12, "TD");
		glBindAttribLocation(program, 13, "MX");
		glBindAttribLocation(program, 14, "TX");

		glLinkProgram(program);
		glUseProgram(program);

		printProgramInfoLog(program);

		lightmixLocation = glGetUniformLocation(program, "lightmix");
		glUniform1f(lightmixLocation, 1.0f); // Initial light mix is full.

		PAlocation = glGetAttribLocation(program, "PA");
		RAlocation = glGetAttribLocation(program, "RA");
		SXlocation = glGetAttribLocation(program, "SX");
		TDlocation = glGetAttribLocation(program, "TD");
		MXlocation = glGetAttribLocation(program, "MX");
		TXlocation = glGetAttribLocation(program, "TX");

		glActiveTexture(GL_TEXTURE0);
		texlocation = glGetUniformLocation(program, "tex0");
		glUniform1i(texlocation, 0);

		glActiveTexture(GL_TEXTURE2);
		envlocation = glGetUniformLocation(program, "env0");
		glUniform1i(envlocation, 2);
	}

	coords3d transform(coords3d position, coords3d angle)
	{
		float xaxis0 = 0;
		float xaxis1 = 0;
		float xaxis2 = 0;

		float yaxis0 = 0;
		float yaxis1 = 0;
		float yaxis2 = 0;

		float zaxis0 = 0;
		float zaxis1 = 0;
		float zaxis2 = 0;

		float cosX = cos(-(angle.x * 0.0174533));
		float cosY = cos(-(angle.y * 0.0174533));
		float cosZ = cos(-(angle.z * 0.0174533));

		float sinX = sin(-(angle.x * 0.0174533));
		float sinY = sin(-(angle.y * 0.0174533));
		float sinZ = sin(-(angle.z * 0.0174533));

		xaxis0 = position.x;											// X-axis rotation
		xaxis1 = (cosX * position.y) - (sinX * position.z);
		xaxis2 = (sinX * position.y) + (cosX * position.z);

		yaxis0 = (cosY * xaxis0) + (sinY * xaxis2);			// Y-axis rotation	// feeds from xaxis0 etc.,
		yaxis1 = xaxis1;
		yaxis2 = (-sinY * xaxis0) + (cosY * xaxis2);

		zaxis0 = (cosZ * yaxis0) - (sinZ * yaxis1);			// Z-axis rotation
		zaxis1 = (sinZ * yaxis0) + (cosZ * yaxis1);
		zaxis2 = yaxis2;

		coords3d r;
		r.x = zaxis0;
		r.y = zaxis1;
		r.z = zaxis2;

		return r;
	}

	void scale(float x, float y, float z, float scale_x, float scale_y, float scale_z, float& fx, float& fy, float& fz)
	{
		fx = x * scale_x;
		fy = y * scale_y;
		fz = z * scale_z;
	}

	void resetimagesto(marker_type handle)
	{
		if (handle.index_type == 1 && handle.isStatic == true)
		{
			for (int i = handle.index + 1; i < image_index + 1; i++)
			{
				moveimage(i, { 0, 0, 0 });
				rotateimage(i, { 0,0,0 });
				scaleimage(i, 0.0);
				planeimage(i, { 0,0,-1 });
				glassimage(i, 0.0);
				autoteximage(i, "", 0.0f, 0.0f);
				multiteximage(i, "", 0.0f, 0.0f);
			}

			int count = object_index - 1;

			for (int i = count; i > -1; i--) // Cycle backwards through objects and roll-back until object handle is no longer affected by image removal.
			{
				if (handle.index <= object_i[i].startimage)
					object_index--;
			}

			image_index = handle.index * 1;
			vert_i = handle.index * 12;
			tex_i = handle.index * 16;
			color_i = handle.index * 16;
		}
		else if (handle.index_type == 1 && handle.isStatic == false)
		{
			for (int i = handle.index + 1; i < dyn_image_index + 1; i++)
			{
				moveimage(i, { 0, 0, 0 });
				rotateimage(i, { 0,0,0 });
				scaleimage(i, 0.0);
				planeimage(i, { 0,0,-1 });
				glassimage(i, 0.0);
				autoteximage(i, "", 0.0f, 0.0f);
				multiteximage(i, "", 0.0f, 0.0f);
			}

			int count = object_index - 1;

			for (int i = count; i > -1; i--) // Cycle backwards through objects and roll-back until object handle is no longer affected by image removal.
			{
				if (handle.index <= object_i[i].startimage)
					object_index--;
			}

			dyn_image_index = handle.index * 1;
			dyn_vert_i = handle.index * 12;
			dyn_tex_i = handle.index * 16;
			dyn_color_i = handle.index * 16;
		}
		else if (handle.index_type != 1)
			cout << "Wrong marker type, expect Main marker.\n";
	}

	void resetZCimagesto(marker_type handle)
	{
		if (handle.index_type == 2 && handle.isStatic == true)
		{
			for (int i = handle.index + 1; i < ZCimage_index + 1; i++)
			{
				moveZCimage(i, { 0, 0, 0 });
				rotateZCimage(i, { 0,0,0 });
				scaleZCimage(i, 0.0);
				planeZCimage(i, { 0,0,-1 });
				glassZCimage(i, 0.0);
				autotexZCimage(i, "", 0.0f, 0.0f);
				multitexZCimage(i, "", 0.0f, 0.0f);
			}

			int count = ZCobject_index - 1;

			for (int i = count; i > -1; i--) // Cycle backwards through ZC objects and roll-back until object handle is no longer affected by image removal.
			{
				if (handle.index <= ZCobject_i[i].startimage)
					ZCobject_index--;
			}

			ZCimage_index = handle.index * 1;
			Zvert_i = handle.index * 12;
			Ztex_i = handle.index * 16;
			Zcolor_i = handle.index * 16;
		}
		else if (handle.index_type == 2 && handle.isStatic == false)
		{
			for (int i = handle.index + 1; i < dyn_ZCimage_index + 1; i++)
			{
				moveZCimage(i, { 0, 0, 0 });
				rotateZCimage(i, { 0,0,0 });
				scaleZCimage(i, 0.0);
				planeZCimage(i, { 0,0,-1 });
				glassZCimage(i, 0.0);
				autotexZCimage(i, "", 0.0f, 0.0f);
				multitexZCimage(i, "", 0.0f, 0.0f);
			}

			int count = ZCobject_index - 1;

			for (int i = count; i > -1; i--) // Cycle backwards through ZC objects and roll-back until object handle is no longer affected by image removal.
			{
				if (handle.index <= ZCobject_i[i].startimage)
					ZCobject_index--;
			}

			dyn_ZCimage_index = handle.index * 1;
			dyn_Zvert_i = handle.index * 12;
			dyn_Ztex_i = handle.index * 16;
			dyn_Zcolor_i = handle.index * 16;
		}
		else if (handle.index_type != 2)
			cout << "Wrong marker type, expected ZC marker.\n";
	}

	void resetHUDimagesto(marker_type handle)
	{
		if (handle.index_type == 3 && handle.isStatic == true)
		{
			for (int i = handle.index + 1; i < HUDimage_index + 1; i++)
			{
				moveHUDimage(i, { 0, 0, 0 });
				rotateHUDimage(i, { 0,0,0 });
				scaleHUDimage(i, 0.0);
				glassHUDimage(i, 0.0);
				autotexHUDimage(i, "", 0.0f, 0.0f);
				multitexHUDimage(i, "", 0.0f, 0.0f);
			}

			int count = HUDobject_index - 1;

			for (int i = count; i > -1; i--) // Cycle backwards through HUD objects and roll-back until object handle is no longer affected by image removal.
			{
				if (handle.index <= HUDobject_i[i].startimage)
					HUDobject_index--;
			}

			HUDimage_index = handle.index * 1;
			Hvert_i = handle.index * 12;
			Htex_i = handle.index * 16;
			Hcolor_i = handle.index * 16;
		}
		else if (handle.index_type == 3 && handle.isStatic == false)
		{
			for (int i = handle.index + 1; i < dyn_HUDimage_index + 1; i++)
			{
				moveHUDimage(i, { 0, 0, 0 });
				rotateHUDimage(i, { 0,0,0 });
				scaleHUDimage(i, 0.0);
				glassHUDimage(i, 0.0);
				autotexHUDimage(i, "", 0.0f, 0.0f);
				multitexHUDimage(i, "", 0.0f, 0.0f);
			}

			int count = HUDobject_index - 1;

			for (int i = count; i > -1; i--) // Cycle backwards through HUD objects and roll-back until object handle is no longer affected by image removal.
			{
				if (handle.index <= HUDobject_i[i].startimage)
					HUDobject_index--;
			}

			dyn_HUDimage_index = handle.index * 1;
			dyn_Hvert_i = handle.index * 12;
			dyn_Htex_i = handle.index * 16;
			dyn_Hcolor_i = handle.index * 16;
		}
		else if (handle.index_type != 3)
			cout << "Wrong marker type, expected HUD marker.\n";
	}

	void setDynamicmode(bool state)
	{
		dyn_mode = state;
	}

	float twoDtoOneD(ipoint p, int xsize) // convert 2D coords to 1D coords.
	{
		return (p.y * xsize) + p.x;
	}

	ipoint oneDtoTwoD(int p, int xsize) // convert 1D coords to 2D coords.
	{
		ipoint p2;

		p2.x = p % xsize;
		p2.y = p / xsize;

		return p2;

		/*
		// 3D version

		i = x + width*y + width*height*z;

		x = i % width;
		y = (i / width)%height;
		z = i / (width*height);

		*/
	}

	int loadobject(string filename)
	{
		object_mutex.lock();

		bool firsttime = true;
		araFormat ara;

		ifstream arafile;
		arafile.open(filename, ios::binary);

		if (arafile.is_open())
		{
			string::size_type sz;

			arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			string version;
			version.resize(sz);
			arafile.read(&version[0], sz);

			arafile.read(reinterpret_cast<char*>(&ara.quad_count), sizeof(int));
			object_i[object_index].count = ara.quad_count;
			checkLimits(filename, 1, ara.quad_count);

			int winding = 0;
			arafile.read(reinterpret_cast<char*>(&winding), sizeof(int)); // Winding

			if (winding == 0)
				ara.m.reverse_winding = false;
			else if (winding == 1)
				ara.m.reverse_winding = true;

			arafile.read(reinterpret_cast<char*>(&ara.m.fresnel), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.envmap), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.alpha), sizeof(float));

			arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			ara.m.autotex_image.resize(sz);
			arafile.read(&ara.m.autotex_image[0], sz);

			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_zoom), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_mixratio), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_offset.x), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_offset.y), sizeof(float));

			arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			ara.m.multitex_image.resize(sz);
			arafile.read(&ara.m.multitex_image[0], sz);

			arafile.read(reinterpret_cast<char*>(&ara.m.multitex_zoom), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.multitex_mixratio), sizeof(float));

			for (int i = 0; i < ara.quad_count; i++)
			{
				arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
				ara.q.tex_name.resize(sz);
				arafile.read(&ara.q.tex_name[0], sz);

				for (int j = 0; j < 4; j++)
				{
					arafile.read(reinterpret_cast<char*>(&ara.q.v[j].x), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.v[j].y), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.v[j].z), sizeof(float));
				}

				for (int j = 0; j < 4; j++)
				{
					arafile.read(reinterpret_cast<char*>(&ara.q.t[j].x), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.t[j].y), sizeof(float));
				}

				for (int j = 0; j < 4; j++)
				{
					arafile.read(reinterpret_cast<char*>(&ara.q.n[j].x), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.n[j].y), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.n[j].z), sizeof(float));
				}

				coords3d col = { 1.0f, 1.0f, 1.0f }; // We default vertex color to "white"

				if (ara.m.reverse_winding == false)
					ara.q.quad_handle = loadimage_patch(ara.q.tex_name, ara.q.v[0], ara.q.v[1], ara.q.v[2], ara.q.v[3], col, col, col, col, ara.q.t[0], ara.q.t[1], ara.q.t[2], ara.q.t[3], ara.q.n[0], ara.q.n[1], ara.q.n[2], ara.q.n[3], ara.m.fresnel, ara.m.envmap, ara.m.alpha);

				else if (ara.m.reverse_winding == true)
					ara.q.quad_handle = loadimage_patch_r(ara.q.tex_name, ara.q.v[0], ara.q.v[1], ara.q.v[2], ara.q.v[3], col, col, col, col, ara.q.t[0], ara.q.t[1], ara.q.t[2], ara.q.t[3], ara.q.n[0], ara.q.n[1], ara.q.n[2], ara.q.n[3], ara.m.fresnel, ara.m.envmap, ara.m.alpha);

				if (firsttime == true)
				{
					firsttime = false;
					object_i[object_index].startimage = ara.q.quad_handle;
				}

				if (ara.m.autotex_image != "")
				{
					autoteximage(ara.q.quad_handle, ara.m.autotex_image, ara.m.autotex_zoom, ara.m.autotex_mixratio);
					scrollImageAutoTexture(ara.q.quad_handle, ara.m.autotex_offset);
				}

				if (ara.m.multitex_image != "")
				{
					multiteximage(ara.q.quad_handle, ara.m.multitex_image, ara.m.multitex_zoom, ara.m.multitex_mixratio);
				}
			}
		}
		else
			cout << filename << " object file not found!\n";

		arafile.close();

		object_index++;
		int oi = object_index;

		object_mutex.unlock();

		return oi;
	}

	int loadobject_ice(string filename)
	{
		object_mutex.lock();

		bool firsttime = true;

		string manifold_name;
		string line;

		coords3d a, b, c, d;
		coords3d col_a, col_b, col_c, col_d;
		ipoint ta, tb, tc, td;
		coords3d na, nb, nc, nd;

		cout << "Load main object: " << filename << "\n";

		ifstream objfile(filename);  // THE PATH "ASSETS" IS BREAKING THE RELOAD FOR ARENA. -bUT PRODUCTION IS COUNTING ON IT.

		if (objfile.is_open())
		{
			std::getline(objfile, line); // Version check
			int fileversion = atoi(line.c_str());

			if (fileversion != 1104)
			{
				cout << "Invalid ice file version - " << filename << "\n";
				objfile.close();
				object_mutex.unlock();
				return -1;
			}

			std::getline(objfile, line); // Manifold count
			object_i[object_index].count = atoi(line.c_str());

			checkLimits(filename, 1, object_i[object_index].count);

			for (int i = 0; i < object_i[object_index].count; i++)
			{
				getline(objfile, manifold_name); // Object group ID (not used in production).
				getline(objfile, manifold_name); // Fast string to int.

				getline(objfile, line);
				a.x = stof(line.c_str());
				getline(objfile, line);
				a.y = stof(line.c_str());
				getline(objfile, line);
				a.z = stof(line.c_str());

				getline(objfile, line);
				b.x = stof(line.c_str());
				getline(objfile, line);
				b.y = stof(line.c_str());
				getline(objfile, line);
				b.z = stof(line.c_str());

				getline(objfile, line);
				c.x = stof(line.c_str());
				getline(objfile, line);
				c.y = stof(line.c_str());
				getline(objfile, line);
				c.z = stof(line.c_str());

				getline(objfile, line);
				d.x = stof(line.c_str());
				getline(objfile, line);
				d.y = stof(line.c_str());
				getline(objfile, line);
				d.z = stof(line.c_str());

				// Vertex Colors

				getline(objfile, line);
				col_a.x = stof(line.c_str());
				getline(objfile, line);
				col_a.y = stof(line.c_str());
				getline(objfile, line);
				col_a.z = stof(line.c_str());

				getline(objfile, line);
				col_b.x = stof(line.c_str());
				getline(objfile, line);
				col_b.y = stof(line.c_str());
				getline(objfile, line);
				col_b.z = stof(line.c_str());

				getline(objfile, line);
				col_c.x = stof(line.c_str());
				getline(objfile, line);
				col_c.y = stof(line.c_str());
				getline(objfile, line);
				col_c.z = stof(line.c_str());

				getline(objfile, line);
				col_d.x = stof(line.c_str());
				getline(objfile, line);
				col_d.y = stof(line.c_str());
				getline(objfile, line);
				col_d.z = stof(line.c_str());

				// Tex Coords

				getline(objfile, line);
				ta.x = atoi(line.c_str());
				getline(objfile, line);
				ta.y = atoi(line.c_str());

				getline(objfile, line);
				tb.x = atoi(line.c_str());
				getline(objfile, line);
				tb.y = atoi(line.c_str());

				getline(objfile, line);
				tc.x = atoi(line.c_str());
				getline(objfile, line);
				tc.y = atoi(line.c_str());

				getline(objfile, line);
				td.x = atoi(line.c_str());
				getline(objfile, line);
				td.y = atoi(line.c_str());

				// Normal Coords

				getline(objfile, line);
				na.x = stof(line.c_str());
				getline(objfile, line);
				na.y = stof(line.c_str());
				getline(objfile, line);
				na.z = stof(line.c_str());

				getline(objfile, line);
				nb.x = stof(line.c_str());
				getline(objfile, line);
				nb.y = stof(line.c_str());
				getline(objfile, line);
				nb.z = stof(line.c_str());

				getline(objfile, line);
				nc.x = stof(line.c_str());
				getline(objfile, line);
				nc.y = stof(line.c_str());
				getline(objfile, line);
				nc.z = stof(line.c_str());

				getline(objfile, line);
				nd.x = stof(line.c_str());
				getline(objfile, line);
				nd.y = stof(line.c_str());
				getline(objfile, line);
				nd.z = stof(line.c_str());

				getline(objfile, line);
				float alpha = stof(line.c_str());

				// .Reverse flag (Use forwarding winding command if true/1)
				getline(objfile, line);
				int r = atoi(line.c_str());

				getline(objfile, line);
				float fresnel = stof(line.c_str());
				getline(objfile, line);
				float envmap = stof(line.c_str());

				int ohandle = 0;

				if (r == 0)
					ohandle = loadimage_patch(manifold_name, a, b, c, d, col_a, col_b, col_c, col_d, ta, tb, tc, td, na, nb, nc, nd, fresnel, envmap, alpha);
				else
					ohandle = loadimage_patch_r(manifold_name, a, b, c, d, col_a, col_b, col_c, col_d, ta, tb, tc, td, na, nb, nc, nd, fresnel, envmap, alpha);

				if (firsttime == true)
				{
					firsttime = false;
					object_i[object_index].startimage = ohandle;
				}
			}
		}
		else
			cout << filename << " object file not found!\n";

		object_index++;

		int oi = object_index;

		object_mutex.unlock();

		return oi;
	}

	void moveobject(int handle, coords3d pos)
	{
		for (int i = 0; i < object_i[handle - 1].count; i++)
			moveimage(object_i[handle - 1].startimage + i, pos);
	}

	void rotateobject(int handle, coords3d angle)
	{
		for (int i = 0; i < object_i[handle - 1].count; i++)
			rotateimage(object_i[handle - 1].startimage + i, angle);
	}

	void Qrotateobject(int handle, quat angle)
	{
		for (int i = 0; i < object_i[handle - 1].count; i++)
			Qrotateimage(object_i[handle - 1].startimage + i, angle);
	}

	void scaleobject(int handle, float scale)
	{
		for (int i = 0; i < object_i[handle - 1].count; i++)
			scaleimage(object_i[handle - 1].startimage + i, scale);
	}

	void glassobject(int handle, float opac)
	{
		for (int i = 0; i < object_i[handle - 1].count; i++)
			glassimage(object_i[handle - 1].startimage + i, opac);
	}

	void planeobject(int handle, coords3d normal)
	{
		for (int i = 0; i < object_i[handle - 1].count; i++)
			planeimage(object_i[handle - 1].startimage + i, normal);
	}

	int loadZCobject(string filename)
	{
		object_mutex.lock();

		bool firsttime = true;
		araFormat ara;

		ifstream arafile;
		arafile.open(filename, ios::binary);

		if (arafile.is_open())
		{
			string::size_type sz;

			arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			string version;
			version.resize(sz);
			arafile.read(&version[0], sz);

			arafile.read(reinterpret_cast<char*>(&ara.quad_count), sizeof(int));
			ZCobject_i[ZCobject_index].count = ara.quad_count;
			checkLimits(filename, 2, ara.quad_count);

			int winding = 0;
			arafile.read(reinterpret_cast<char*>(&winding), sizeof(int)); // Winding

			if (winding == 0)
				ara.m.reverse_winding = false;
			else if (winding == 1)
				ara.m.reverse_winding = true;

			arafile.read(reinterpret_cast<char*>(&ara.m.fresnel), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.envmap), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.alpha), sizeof(float));

			arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			ara.m.autotex_image.resize(sz);
			arafile.read(&ara.m.autotex_image[0], sz);

			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_zoom), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_mixratio), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_offset.x), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_offset.y), sizeof(float));

			arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			ara.m.multitex_image.resize(sz);
			arafile.read(&ara.m.multitex_image[0], sz);

			arafile.read(reinterpret_cast<char*>(&ara.m.multitex_zoom), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.multitex_mixratio), sizeof(float));

			for (int i = 0; i < ara.quad_count; i++)
			{
				arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
				ara.q.tex_name.resize(sz);
				arafile.read(&ara.q.tex_name[0], sz);

				for (int j = 0; j < 4; j++)
				{
					arafile.read(reinterpret_cast<char*>(&ara.q.v[j].x), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.v[j].y), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.v[j].z), sizeof(float));
				}

				for (int j = 0; j < 4; j++)
				{
					arafile.read(reinterpret_cast<char*>(&ara.q.t[j].x), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.t[j].y), sizeof(float));
				}

				for (int j = 0; j < 4; j++)
				{
					arafile.read(reinterpret_cast<char*>(&ara.q.n[j].x), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.n[j].y), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.n[j].z), sizeof(float));
				}

				coords3d col = { 1.0f, 1.0f, 1.0f }; // We default vertex color to "white"

				if (ara.m.reverse_winding == false)
					ara.q.quad_handle = loadZCimage_patch(ara.q.tex_name, ara.q.v[0], ara.q.v[1], ara.q.v[2], ara.q.v[3], col, col, col, col, ara.q.t[0], ara.q.t[1], ara.q.t[2], ara.q.t[3], ara.q.n[0], ara.q.n[1], ara.q.n[2], ara.q.n[3], ara.m.fresnel, ara.m.envmap, ara.m.alpha);

				else if (ara.m.reverse_winding == true)
					ara.q.quad_handle = loadZCimage_patch_r(ara.q.tex_name, ara.q.v[0], ara.q.v[1], ara.q.v[2], ara.q.v[3], col, col, col, col, ara.q.t[0], ara.q.t[1], ara.q.t[2], ara.q.t[3], ara.q.n[0], ara.q.n[1], ara.q.n[2], ara.q.n[3], ara.m.fresnel, ara.m.envmap, ara.m.alpha);

				if (firsttime == true)
				{
					firsttime = false;
					ZCobject_i[ZCobject_index].startimage = ara.q.quad_handle;
				}

				if (ara.m.autotex_image != "")
				{
					autotexZCimage(ara.q.quad_handle, ara.m.autotex_image, ara.m.autotex_zoom, ara.m.autotex_mixratio);
					scrollZCImageAutoTexture(ara.q.quad_handle, ara.m.autotex_offset);
				}

				if (ara.m.multitex_image != "")
				{
					multitexZCimage(ara.q.quad_handle, ara.m.multitex_image, ara.m.multitex_zoom, ara.m.multitex_mixratio);
				}
			}
		}
		else
			cout << filename << " ZCobject file not found!\n";

		arafile.close();

		ZCobject_index++;
		int oi = ZCobject_index;

		object_mutex.unlock();

		return oi;
	}

	int loadZCobject_ice(string filename)
	{
		object_mutex.lock();

		bool firsttime = true;

		string manifold_name;
		string line;

		coords3d a, b, c, d;
		coords3d col_a, col_b, col_c, col_d;
		ipoint ta, tb, tc, td;
		coords3d na, nb, nc, nd;

		cout << "Loading ZC object = " << filename << "\n";

		ifstream objfile(filename); // THE PATH "ASSETS" IS BREAKING THE RELOAD FOR ARENA. -bUT PRODUCTION IS COUNTING ON IT.

		if (objfile.is_open())
		{
			std::getline(objfile, line); // Version check
			int fileversion = atoi(line.c_str());

			if (fileversion != 1104)
			{
				cout << "Invalid file version - " << filename << "\n";
				objfile.close();
				object_mutex.unlock();
				return -1;
			}

			std::getline(objfile, line); // Manifold count
			ZCobject_i[ZCobject_index].count = atoi(line.c_str());

			checkLimits(filename, 2, ZCobject_i[ZCobject_index].count);

			for (int i = 0; i < ZCobject_i[ZCobject_index].count; i++)
			{
				getline(objfile, manifold_name); // Object group ID (not used in production).
				getline(objfile, manifold_name); // Fast string to int.

				getline(objfile, line);
				a.x = stof(line.c_str());
				getline(objfile, line);
				a.y = stof(line.c_str());
				getline(objfile, line);
				a.z = stof(line.c_str());

				getline(objfile, line);
				b.x = stof(line.c_str());
				getline(objfile, line);
				b.y = stof(line.c_str());
				getline(objfile, line);
				b.z = stof(line.c_str());

				getline(objfile, line);
				c.x = stof(line.c_str());
				getline(objfile, line);
				c.y = stof(line.c_str());
				getline(objfile, line);
				c.z = stof(line.c_str());

				getline(objfile, line);
				d.x = stof(line.c_str());
				getline(objfile, line);
				d.y = stof(line.c_str());
				getline(objfile, line);
				d.z = stof(line.c_str());

				// Vertex Colors

				getline(objfile, line);
				col_a.x = stof(line.c_str());
				getline(objfile, line);
				col_a.y = stof(line.c_str());
				getline(objfile, line);
				col_a.z = stof(line.c_str());

				getline(objfile, line);
				col_b.x = stof(line.c_str());
				getline(objfile, line);
				col_b.y = stof(line.c_str());
				getline(objfile, line);
				col_b.z = stof(line.c_str());

				getline(objfile, line);
				col_c.x = stof(line.c_str());
				getline(objfile, line);
				col_c.y = stof(line.c_str());
				getline(objfile, line);
				col_c.z = stof(line.c_str());

				getline(objfile, line);
				col_d.x = stof(line.c_str());
				getline(objfile, line);
				col_d.y = stof(line.c_str());
				getline(objfile, line);
				col_d.z = stof(line.c_str());

				// Tex Coords

				getline(objfile, line);
				ta.x = atoi(line.c_str());
				getline(objfile, line);
				ta.y = atoi(line.c_str());

				getline(objfile, line);
				tb.x = atoi(line.c_str());
				getline(objfile, line);
				tb.y = atoi(line.c_str());

				getline(objfile, line);
				tc.x = atoi(line.c_str());
				getline(objfile, line);
				tc.y = atoi(line.c_str());

				getline(objfile, line);
				td.x = atoi(line.c_str());
				getline(objfile, line);
				td.y = atoi(line.c_str());

				// Normal Coords

				getline(objfile, line);
				na.x = stof(line.c_str());
				getline(objfile, line);
				na.y = stof(line.c_str());
				getline(objfile, line);
				na.z = stof(line.c_str());

				getline(objfile, line);
				nb.x = stof(line.c_str());
				getline(objfile, line);
				nb.y = stof(line.c_str());
				getline(objfile, line);
				nb.z = stof(line.c_str());

				getline(objfile, line);
				nc.x = stof(line.c_str());
				getline(objfile, line);
				nc.y = stof(line.c_str());
				getline(objfile, line);
				nc.z = stof(line.c_str());

				getline(objfile, line);
				nd.x = stof(line.c_str());
				getline(objfile, line);
				nd.y = stof(line.c_str());
				getline(objfile, line);
				nd.z = stof(line.c_str());

				getline(objfile, line);
				float alpha = stof(line.c_str());

				// .Reverse flag (Use forwarding winding command if true/1)

				getline(objfile, line);
				int r = atoi(line.c_str());

				getline(objfile, line);
				float fresnel = stof(line.c_str());
				getline(objfile, line);
				float envmap = stof(line.c_str());

				int ohandle = 0;

				if (r == 0)
					ohandle = loadZCimage_patch(manifold_name, a, b, c, d, col_a, col_b, col_c, col_d, ta, tb, tc, td, na, nb, nc, nd, fresnel, envmap, alpha);
				else
					ohandle = loadZCimage_patch_r(manifold_name, a, b, c, d, col_a, col_b, col_c, col_d, ta, tb, tc, td, na, nb, nc, nd, fresnel, envmap, alpha);

				if (firsttime == true)
				{
					firsttime = false;
					ZCobject_i[ZCobject_index].startimage = ohandle;
				}
			}
		}
		else
			cout << filename << " object file not found!\n";

		ZCobject_index++;

		int oi = ZCobject_index;

		object_mutex.unlock();

		return oi;
	}

	void moveZCobject(int handle, coords3d pos)
	{
		for (int i = 0; i < ZCobject_i[handle - 1].count; i++)
			moveZCimage(ZCobject_i[handle - 1].startimage + i, pos);
	}

	void rotateZCobject(int handle, coords3d angle)
	{
		for (int i = 0; i < ZCobject_i[handle - 1].count; i++)
			rotateZCimage(ZCobject_i[handle - 1].startimage + i, angle);
	}

	void QrotateZCobject(int handle, quat angle)
	{
		for (int i = 0; i < ZCobject_i[handle - 1].count; i++)
			QrotateZCimage(ZCobject_i[handle - 1].startimage + i, angle);
	}

	void scaleZCobject(int handle, float scale)
	{
		for (int i = 0; i < ZCobject_i[handle - 1].count; i++)
			scaleZCimage(ZCobject_i[handle - 1].startimage + i, scale);
	}

	void glassZCobject(int handle, float opac)
	{
		for (int i = 0; i < ZCobject_i[handle - 1].count; i++)
			glassZCimage(ZCobject_i[handle - 1].startimage + i, opac);
	}

	void planeZCobject(int handle, coords3d normal)
	{
		for (int i = 0; i < ZCobject_i[handle - 1].count; i++)
			planeZCimage(ZCobject_i[handle - 1].startimage + i, normal);
	}

	int loadimage_patch(string filename, coords3d a, coords3d b, coords3d c, coords3d d, coords3d col_a, coords3d col_b, coords3d col_c, coords3d col_d, ipoint ta, ipoint tb, ipoint tc, ipoint td, coords3d na, coords3d nb, coords3d nc, coords3d nd, float fresnel, float envmap, float alpha)
	{
		multimap<string, int>::iterator it = atlas_map.find(filename);

		if (it == atlas_map.end())
		{
			if (failedtofind != filename)
			{
				if (global_suppressImages == false)
					cout << "Failed to find " << filename << " in atlas!\n";
				//failedtofind = filename;
				//filename = "white0";
				it = atlas_map.find("white0");
			}
			//it = atlas_map.find("black");
			//shutdown();
		}

		int previous_vert_i = 0;
		int previous_tex_i = 0;
		int previous_color_i = 0;
		int previous_image_index = 0;

		if (dyn_mode == true)
		{
			previous_vert_i = vert_i;
			previous_tex_i = tex_i;
			previous_color_i = color_i;

			previous_image_index = image_index;

			vert_i = dyn_vert_i;
			tex_i = dyn_tex_i;
			color_i = dyn_color_i;
			image_index = dyn_image_index;
		}

		image_tex_i[image_index] = it->second;

		image_size[image_index].x = atlas_xsize[image_tex_i[image_index]];
		image_size[image_index].y = atlas_ysize[image_tex_i[image_index]];

		// vertex 1

		vertex_array[vert_i + 0] = d.x;
		vertex_array[vert_i + 1] = d.y;
		vertex_array[vert_i + 2] = d.z;

		texcoord_array[tex_i + 0] = twoDtoOneD({ atlas_x[image_tex_i[image_index]] + td.x , (atlas_y[image_tex_i[image_index]] + atlas_ysize[image_tex_i[image_index]]) + td.y }, 2048);
		texcoord_array[tex_i + 1] = displacementD;
		texcoord_array[tex_i + 2] = fresnel;
		texcoord_array[tex_i + 3] = envmap;

		color_array[color_i + 0] = col_d.x;
		color_array[color_i + 1] = col_d.y;
		color_array[color_i + 2] = col_d.z;
		color_array[color_i + 3] = alpha;

		normal_array[vert_i + 0] = nd.x;
		normal_array[vert_i + 1] = nd.y;
		normal_array[vert_i + 2] = nd.z;


		// vertex 2

		vertex_array[vert_i + 3] = c.x;
		vertex_array[vert_i + 4] = c.y;
		vertex_array[vert_i + 5] = c.z;

		texcoord_array[tex_i + 4] = twoDtoOneD({ (atlas_x[image_tex_i[image_index]] + atlas_xsize[image_tex_i[image_index]]) + tc.x , (atlas_y[image_tex_i[image_index]] + atlas_ysize[image_tex_i[image_index]]) + tc.y }, 2048);
		texcoord_array[tex_i + 5] = displacementC;
		texcoord_array[tex_i + 6] = fresnel;
		texcoord_array[tex_i + 7] = envmap;

		color_array[color_i + 4] = col_c.x;
		color_array[color_i + 5] = col_c.y;
		color_array[color_i + 6] = col_c.z;
		color_array[color_i + 7] = alpha;

		normal_array[vert_i + 3] = nc.x;
		normal_array[vert_i + 4] = nc.y;
		normal_array[vert_i + 5] = nc.z;


		// vertex 3

		vertex_array[vert_i + 6] = b.x;
		vertex_array[vert_i + 7] = b.y;
		vertex_array[vert_i + 8] = b.z;

		texcoord_array[tex_i + 8] = twoDtoOneD({ (atlas_x[image_tex_i[image_index]] + atlas_xsize[image_tex_i[image_index]]) + tb.x , atlas_y[image_tex_i[image_index]] + tb.y }, 2048);
		texcoord_array[tex_i + 9] = displacementB;
		texcoord_array[tex_i + 10] = fresnel;
		texcoord_array[tex_i + 11] = envmap;

		color_array[color_i + 8] = col_b.x;
		color_array[color_i + 9] = col_b.y;
		color_array[color_i + 10] = col_b.z;
		color_array[color_i + 11] = alpha;

		normal_array[vert_i + 6] = nb.x;
		normal_array[vert_i + 7] = nb.y;
		normal_array[vert_i + 8] = nb.z;


		// vertex 4

		vertex_array[vert_i + 9] = a.x;
		vertex_array[vert_i + 10] = a.y;
		vertex_array[vert_i + 11] = a.z;

		texcoord_array[tex_i + 12] = twoDtoOneD({ atlas_x[image_tex_i[image_index]] + ta.x , atlas_y[image_tex_i[image_index]] + ta.y }, 2048);
		texcoord_array[tex_i + 13] = displacementA;
		texcoord_array[tex_i + 14] = fresnel;
		texcoord_array[tex_i + 15] = envmap;

		color_array[color_i + 12] = col_a.x;
		color_array[color_i + 13] = col_a.y;
		color_array[color_i + 14] = col_a.z;
		color_array[color_i + 15] = alpha;

		normal_array[vert_i + 9] = na.x;
		normal_array[vert_i + 10] = na.y;
		normal_array[vert_i + 11] = na.z;

		scaleimage(image_index + 1, 1.0);
		planeimage(image_index + 1, { 0,0,-1 });

		image_index++;
		int ii = image_index;

		if (dyn_mode == false)
		{
			vert_i = vert_i + 12;
			tex_i = tex_i + 16;
			color_i = color_i + 16;
		}
		else
		{
			dyn_image_index = image_index;
			dyn_vert_i = vert_i + 12;
			dyn_tex_i = tex_i + 16;
			dyn_color_i = color_i + 16;

			vert_i = previous_vert_i;
			tex_i = previous_tex_i;
			color_i = previous_color_i;
			image_index = previous_image_index;
		}

		updateVBO = true;

		return ii;
	}

	int loadimage_patch_r(string filename, coords3d a, coords3d b, coords3d c, coords3d d, coords3d col_a, coords3d col_b, coords3d col_c, coords3d col_d, ipoint ta, ipoint tb, ipoint tc, ipoint td, coords3d na, coords3d nb, coords3d nc, coords3d nd, float fresnel, float envmap, float alpha)
	{
		multimap<string, int>::iterator it = atlas_map.find(filename);

		if (it == atlas_map.end())
		{
			if (failedtofind != filename)
			{
				if (global_suppressImages == false)
					cout << "Failed to find " << filename << " in atlas!\n";
				//failedtofind = filename;
				it = atlas_map.find("white0");
			}
			//it = atlas_map.find("black");
			//shutdown();
		}

		int previous_vert_i = 0;
		int previous_tex_i = 0;
		int previous_color_i = 0;
		int previous_image_index = 0;

		if (dyn_mode == true)
		{
			previous_vert_i = vert_i;
			previous_tex_i = tex_i;
			previous_color_i = color_i;

			previous_image_index = image_index;

			vert_i = dyn_vert_i;
			tex_i = dyn_tex_i;
			color_i = dyn_color_i;
			image_index = dyn_image_index;
		}

		image_tex_i[image_index] = it->second;

		image_size[image_index].x = atlas_xsize[image_tex_i[image_index]];
		image_size[image_index].y = atlas_ysize[image_tex_i[image_index]];

		// vertex 1

		vertex_array[vert_i + 0] = a.x;
		vertex_array[vert_i + 1] = a.y;
		vertex_array[vert_i + 2] = a.z;

		texcoord_array[tex_i + 0] = twoDtoOneD({ atlas_x[image_tex_i[image_index]] + ta.x , atlas_y[image_tex_i[image_index]] + ta.y }, 2048);
		texcoord_array[tex_i + 1] = displacementA;
		texcoord_array[tex_i + 2] = fresnel;
		texcoord_array[tex_i + 3] = envmap;

		color_array[color_i + 0] = col_a.x;
		color_array[color_i + 1] = col_a.y;
		color_array[color_i + 2] = col_a.z;
		color_array[color_i + 3] = alpha;

		normal_array[vert_i + 0] = -na.x;
		normal_array[vert_i + 1] = -na.y;
		normal_array[vert_i + 2] = -na.z;


		// vertex 2

		vertex_array[vert_i + 3] = b.x;
		vertex_array[vert_i + 4] = b.y;
		vertex_array[vert_i + 5] = b.z;

		texcoord_array[tex_i + 4] = twoDtoOneD({ (atlas_x[image_tex_i[image_index]] + atlas_xsize[image_tex_i[image_index]]) + tb.x , atlas_y[image_tex_i[image_index]] + tb.y }, 2048);
		texcoord_array[tex_i + 5] = displacementB;
		texcoord_array[tex_i + 6] = fresnel;
		texcoord_array[tex_i + 7] = envmap;

		color_array[color_i + 4] = col_b.x;
		color_array[color_i + 5] = col_b.y;
		color_array[color_i + 6] = col_b.z;
		color_array[color_i + 7] = alpha;

		normal_array[vert_i + 3] = -nb.x;
		normal_array[vert_i + 4] = -nb.y;
		normal_array[vert_i + 5] = -nb.z;


		// vertex 3

		vertex_array[vert_i + 6] = c.x;
		vertex_array[vert_i + 7] = c.y;
		vertex_array[vert_i + 8] = c.z;

		texcoord_array[tex_i + 8] = twoDtoOneD({ (atlas_x[image_tex_i[image_index]] + atlas_xsize[image_tex_i[image_index]]) + tc.x , (atlas_y[image_tex_i[image_index]] + atlas_ysize[image_tex_i[image_index]]) + tc.y }, 2048);
		texcoord_array[tex_i + 9] = displacementC;
		texcoord_array[tex_i + 10] = fresnel;
		texcoord_array[tex_i + 11] = envmap;

		color_array[color_i + 8] = col_c.x;
		color_array[color_i + 9] = col_c.y;
		color_array[color_i + 10] = col_c.z;
		color_array[color_i + 11] = alpha;

		normal_array[vert_i + 6] = -nc.x;
		normal_array[vert_i + 7] = -nc.y;
		normal_array[vert_i + 8] = -nc.z;


		// vertex 4

		vertex_array[vert_i + 9] = d.x;
		vertex_array[vert_i + 10] = d.y;
		vertex_array[vert_i + 11] = d.z;

		texcoord_array[tex_i + 12] = twoDtoOneD({ atlas_x[image_tex_i[image_index]] + td.x , (atlas_y[image_tex_i[image_index]] + atlas_ysize[image_tex_i[image_index]]) + td.y }, 2048);
		texcoord_array[tex_i + 13] = displacementD;
		texcoord_array[tex_i + 14] = fresnel;
		texcoord_array[tex_i + 15] = envmap;

		color_array[color_i + 12] = col_d.x;
		color_array[color_i + 13] = col_d.y;
		color_array[color_i + 14] = col_d.z;
		color_array[color_i + 15] = alpha;

		normal_array[vert_i + 9] = -nd.x;
		normal_array[vert_i + 10] = -nd.y;
		normal_array[vert_i + 11] = -nd.z;

		scaleimage(image_index + 1, 1.0);
		planeimage(image_index + 1, { 0,0,-1 });

		image_index++;
		int ii = image_index;

		if (dyn_mode == false)
		{
			vert_i = vert_i + 12;
			tex_i = tex_i + 16;
			color_i = color_i + 16;
		}
		else
		{
			dyn_image_index = image_index;
			dyn_vert_i = vert_i + 12;
			dyn_tex_i = tex_i + 16;
			dyn_color_i = color_i + 16;

			vert_i = previous_vert_i;
			tex_i = previous_tex_i;
			color_i = previous_color_i;
			image_index = previous_image_index;
		}

		updateVBO = true;

		return ii;
	}

	void updateimage_patch(int handle, string filename, coords3d a, coords3d b, coords3d c, coords3d d, coords3d col_a, coords3d col_b, coords3d col_c, coords3d col_d, ipoint ta, ipoint tb, ipoint tc, ipoint td, float fresnel, float envmap, float alpha)
	{
		multimap<string, int>::iterator it = atlas_map.find(filename);

		if (it == atlas_map.end())
		{
			if (failedtofind != filename)
			{
				cout << "Failed to find " << filename << " in atlas!\n";
				//failedtofind = filename;
				it = atlas_map.find("white0");
			}
			//it = atlas_map.find("black");
			//shutdown();
		}

		image_tex_i[handle - 1] = it->second;

		int Uvert_i = ((handle - 1) * 12);
		int Utex_i = ((handle - 1) * 16);
		int Ucolor_i = ((handle - 1) * 16);

		// vertex 1

		vertex_array[Uvert_i + 0] = d.x;
		vertex_array[Uvert_i + 1] = d.y;
		vertex_array[Uvert_i + 2] = d.z;

		texcoord_array[Utex_i + 0] = twoDtoOneD({ atlas_x[image_tex_i[handle - 1]] + td.x , (atlas_y[image_tex_i[handle - 1]] + atlas_ysize[image_tex_i[handle - 1]]) + td.y }, 2048);
		texcoord_array[Utex_i + 1] = displacementD;
		texcoord_array[Utex_i + 2] = fresnel;
		texcoord_array[Utex_i + 3] = envmap;

		color_array[Ucolor_i + 0] = col_d.x;
		color_array[Ucolor_i + 1] = col_d.y;
		color_array[Ucolor_i + 2] = col_d.z;
		color_array[Ucolor_i + 3] = alpha;


		// vertex 2

		vertex_array[Uvert_i + 3] = c.x;
		vertex_array[Uvert_i + 4] = c.y;
		vertex_array[Uvert_i + 5] = c.z;

		texcoord_array[Utex_i + 4] = twoDtoOneD({ (atlas_x[image_tex_i[handle - 1]] + atlas_xsize[image_tex_i[handle - 1]]) + tc.x , (atlas_y[image_tex_i[handle - 1]] + atlas_ysize[image_tex_i[handle - 1]]) + tc.y }, 2048);
		texcoord_array[Utex_i + 5] = displacementC;
		texcoord_array[Utex_i + 6] = fresnel;
		texcoord_array[Utex_i + 7] = envmap;

		color_array[Ucolor_i + 4] = col_c.x;
		color_array[Ucolor_i + 5] = col_c.y;
		color_array[Ucolor_i + 6] = col_c.z;
		color_array[Ucolor_i + 7] = alpha;


		// vertex 3

		vertex_array[Uvert_i + 6] = b.x;
		vertex_array[Uvert_i + 7] = b.y;
		vertex_array[Uvert_i + 8] = b.z;

		texcoord_array[Utex_i + 8] = twoDtoOneD({ (atlas_x[image_tex_i[handle - 1]] + atlas_xsize[image_tex_i[handle - 1]]) + tb.x , atlas_y[image_tex_i[handle - 1]] + tb.y }, 2048);
		texcoord_array[Utex_i + 9] = displacementB;
		texcoord_array[Utex_i + 10] = fresnel;
		texcoord_array[Utex_i + 11] = envmap;

		color_array[Ucolor_i + 8] = col_b.x;
		color_array[Ucolor_i + 9] = col_b.y;
		color_array[Ucolor_i + 10] = col_b.z;
		color_array[Ucolor_i + 11] = alpha;


		// vertex 4

		vertex_array[Uvert_i + 9] = a.x;
		vertex_array[Uvert_i + 10] = a.y;
		vertex_array[Uvert_i + 11] = a.z;

		texcoord_array[Utex_i + 12] = twoDtoOneD({ atlas_x[image_tex_i[handle - 1]] + ta.x , atlas_y[image_tex_i[handle - 1]] + ta.y }, 2048);
		texcoord_array[Utex_i + 13] = displacementA;
		texcoord_array[Utex_i + 14] = fresnel;
		texcoord_array[Utex_i + 15] = envmap;

		color_array[Ucolor_i + 12] = col_a.x;
		color_array[Ucolor_i + 13] = col_a.y;
		color_array[Ucolor_i + 14] = col_a.z;
		color_array[Ucolor_i + 15] = alpha;

		scaleimage(handle, 1.0);
		planeimage(handle, { 0,0,-1 });

		updateVBO = true;
	}

	void updateimage_patch_r(int handle, string filename, coords3d a, coords3d b, coords3d c, coords3d d, coords3d col_a, coords3d col_b, coords3d col_c, coords3d col_d, ipoint ta, ipoint tb, ipoint tc, ipoint td, float fresnel, float envmap, float alpha)
	{
		multimap<string, int>::iterator it = atlas_map.find(filename);

		if (it == atlas_map.end())
		{
			if (failedtofind != filename)
			{
				if (global_suppressImages == false)
					cout << "Failed to find " << filename << " in atlas!\n";
				//failedtofind = filename;
				it = atlas_map.find("white0");
			}
			//it = atlas_map.find("black");
			//shutdown();
		}


		image_tex_i[handle - 1] = it->second;

		int Uvert_i = ((handle - 1) * 12);
		int Utex_i = ((handle - 1) * 16);
		int Ucolor_i = ((handle - 1) * 16);

		// vertex 1

		vertex_array[Uvert_i + 0] = a.x;
		vertex_array[Uvert_i + 1] = a.y;
		vertex_array[Uvert_i + 2] = a.z;

		texcoord_array[Utex_i + 0] = twoDtoOneD({ atlas_x[image_tex_i[handle - 1]] + ta.x , atlas_y[image_tex_i[handle - 1]] + ta.y }, 2048);
		texcoord_array[Utex_i + 1] = displacementA;
		texcoord_array[Utex_i + 2] = fresnel;
		texcoord_array[Utex_i + 3] = envmap;

		color_array[Ucolor_i + 0] = col_a.x;
		color_array[Ucolor_i + 1] = col_a.y;
		color_array[Ucolor_i + 2] = col_a.z;
		color_array[Ucolor_i + 3] = alpha;


		// vertex 2

		vertex_array[Uvert_i + 3] = b.x;
		vertex_array[Uvert_i + 4] = b.y;
		vertex_array[Uvert_i + 5] = b.z;

		texcoord_array[Utex_i + 4] = twoDtoOneD({ (atlas_x[image_tex_i[handle - 1]] + atlas_xsize[image_tex_i[handle - 1]]) + tb.x , atlas_y[image_tex_i[handle - 1]] + tb.y }, 2048);
		texcoord_array[Utex_i + 5] = displacementB;
		texcoord_array[Utex_i + 6] = fresnel;
		texcoord_array[Utex_i + 7] = envmap;

		color_array[Ucolor_i + 4] = col_b.x;
		color_array[Ucolor_i + 5] = col_b.y;
		color_array[Ucolor_i + 6] = col_b.z;
		color_array[Ucolor_i + 7] = alpha;


		// vertex 3

		vertex_array[Uvert_i + 6] = c.x;
		vertex_array[Uvert_i + 7] = c.y;
		vertex_array[Uvert_i + 8] = c.z;

		texcoord_array[Utex_i + 8] = twoDtoOneD({ (atlas_x[image_tex_i[handle - 1]] + atlas_xsize[image_tex_i[handle - 1]]) + tc.x , (atlas_y[image_tex_i[handle - 1]] + atlas_ysize[image_tex_i[handle - 1]]) + tc.y }, 2048);
		texcoord_array[Utex_i + 9] = displacementC;
		texcoord_array[Utex_i + 10] = fresnel;
		texcoord_array[Utex_i + 11] = envmap;

		color_array[Ucolor_i + 8] = col_c.x;
		color_array[Ucolor_i + 9] = col_c.y;
		color_array[Ucolor_i + 10] = col_c.z;
		color_array[Ucolor_i + 11] = alpha;


		// vertex 4

		vertex_array[Uvert_i + 9] = d.x;
		vertex_array[Uvert_i + 10] = d.y;
		vertex_array[Uvert_i + 11] = d.z;

		texcoord_array[Utex_i + 12] = twoDtoOneD({ atlas_x[image_tex_i[handle - 1]] + td.x , (atlas_y[image_tex_i[handle - 1]] + atlas_ysize[image_tex_i[handle - 1]]) + td.y }, 2048);
		texcoord_array[Utex_i + 13] = displacementD;
		texcoord_array[Utex_i + 14] = fresnel;
		texcoord_array[Utex_i + 15] = envmap;

		color_array[Ucolor_i + 12] = col_d.x;
		color_array[Ucolor_i + 13] = col_d.y;
		color_array[Ucolor_i + 14] = col_d.z;
		color_array[Ucolor_i + 15] = alpha;

		scaleimage(handle, 1.0);
		planeimage(handle, { 0,0,-1 });

		updateVBO = true;
	}

	int loadZCimage_patch(string filename, coords3d a, coords3d b, coords3d c, coords3d d, coords3d col_a, coords3d col_b, coords3d col_c, coords3d col_d, ipoint ta, ipoint tb, ipoint tc, ipoint td, coords3d na, coords3d nb, coords3d nc, coords3d nd, float fresnel, float envmap, float alpha)
	{
		multimap<string, int>::iterator it = atlas_map.find(filename);

		if (it == atlas_map.end())
		{
			if (failedtofind != filename)
			{
				if (global_suppressImages == false)
					cout << "Failed to find " << filename << " in atlas!\n";
				//failedtofind = filename;
				it = atlas_map.find("white0");
			}
			//it = atlas_map.find("black");
			//shutdown();
		}

		int previous_Zvert_i = 0;
		int previous_Ztex_i = 0;
		int previous_Zcolor_i = 0;
		int previous_ZCimage_index = 0;

		if (dyn_mode == true)
		{
			previous_Zvert_i = Zvert_i;
			previous_Ztex_i = Ztex_i;
			previous_Zcolor_i = Zcolor_i;

			previous_ZCimage_index = ZCimage_index;

			Zvert_i = dyn_Zvert_i;
			Ztex_i = dyn_Ztex_i;
			Zcolor_i = dyn_Zcolor_i;
			ZCimage_index = dyn_ZCimage_index;
		}

		ZCimage_tex_i[ZCimage_index] = it->second;

		// vertex 1

		vertex_array[Zvert_i + ZC_vertnorm_offset + 0] = d.x; // 3 * 4 = 12 * 4500 = 54,000
		vertex_array[Zvert_i + ZC_vertnorm_offset + 1] = d.y; //         12 * 5500 = 66,000
		vertex_array[Zvert_i + ZC_vertnorm_offset + 2] = d.z;

		texcoord_array[Ztex_i + ZC_texcolor_offset + 0] = twoDtoOneD({ atlas_x[ZCimage_tex_i[ZCimage_index]] + td.x , atlas_y[ZCimage_tex_i[ZCimage_index]] + atlas_ysize[ZCimage_tex_i[ZCimage_index]] + td.y }, 2048);
		texcoord_array[Ztex_i + ZC_texcolor_offset + 1] = displacementD;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 2] = fresnel;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 3] = envmap;

		color_array[Zcolor_i + ZC_texcolor_offset + 0] = col_d.x;
		color_array[Zcolor_i + ZC_texcolor_offset + 1] = col_d.y;
		color_array[Zcolor_i + ZC_texcolor_offset + 2] = col_d.z;
		color_array[Zcolor_i + ZC_texcolor_offset + 3] = alpha;

		normal_array[Zvert_i + ZC_vertnorm_offset + 0] = nd.x;
		normal_array[Zvert_i + ZC_vertnorm_offset + 1] = nd.y;
		normal_array[Zvert_i + ZC_vertnorm_offset + 2] = nd.z;


		// vertex 2

		vertex_array[Zvert_i + ZC_vertnorm_offset + 3] = c.x;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 4] = c.y;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 5] = c.z;

		texcoord_array[Ztex_i + ZC_texcolor_offset + 4] = twoDtoOneD({ atlas_x[ZCimage_tex_i[ZCimage_index]] + atlas_xsize[ZCimage_tex_i[ZCimage_index]] + tc.x , atlas_y[ZCimage_tex_i[ZCimage_index]] + atlas_ysize[ZCimage_tex_i[ZCimage_index]] + tc.y }, 2048);
		texcoord_array[Ztex_i + ZC_texcolor_offset + 5] = displacementC;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 6] = fresnel;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 7] = envmap;

		color_array[Zcolor_i + ZC_texcolor_offset + 4] = col_c.x;
		color_array[Zcolor_i + ZC_texcolor_offset + 5] = col_c.y;
		color_array[Zcolor_i + ZC_texcolor_offset + 6] = col_c.z;
		color_array[Zcolor_i + ZC_texcolor_offset + 7] = alpha;

		normal_array[Zvert_i + ZC_vertnorm_offset + 3] = nc.x;
		normal_array[Zvert_i + ZC_vertnorm_offset + 4] = nc.y;
		normal_array[Zvert_i + ZC_vertnorm_offset + 5] = nc.z;


		// vertex 3

		vertex_array[Zvert_i + ZC_vertnorm_offset + 6] = b.x;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 7] = b.y;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 8] = b.z;

		texcoord_array[Ztex_i + ZC_texcolor_offset + 8] = twoDtoOneD({ atlas_x[ZCimage_tex_i[ZCimage_index]] + atlas_xsize[ZCimage_tex_i[ZCimage_index]] + tb.x , atlas_y[ZCimage_tex_i[ZCimage_index]] + tb.y }, 2048);
		texcoord_array[Ztex_i + ZC_texcolor_offset + 9] = displacementB;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 10] = fresnel;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 11] = envmap;

		color_array[Zcolor_i + ZC_texcolor_offset + 8] = col_b.x;
		color_array[Zcolor_i + ZC_texcolor_offset + 9] = col_b.y;
		color_array[Zcolor_i + ZC_texcolor_offset + 10] = col_b.z;
		color_array[Zcolor_i + ZC_texcolor_offset + 11] = alpha;

		normal_array[Zvert_i + ZC_vertnorm_offset + 6] = nb.x;
		normal_array[Zvert_i + ZC_vertnorm_offset + 7] = nb.y;
		normal_array[Zvert_i + ZC_vertnorm_offset + 8] = nb.z;


		// vertex 4

		vertex_array[Zvert_i + ZC_vertnorm_offset + 9] = a.x;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 10] = a.y;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 11] = a.z;

		texcoord_array[Ztex_i + ZC_texcolor_offset + 12] = twoDtoOneD({ atlas_x[ZCimage_tex_i[ZCimage_index]] + ta.x , atlas_y[ZCimage_tex_i[ZCimage_index]] + ta.y }, 2048);
		texcoord_array[Ztex_i + ZC_texcolor_offset + 13] = displacementA;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 14] = fresnel;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 15] = envmap;

		color_array[Zcolor_i + ZC_texcolor_offset + 12] = col_a.x;
		color_array[Zcolor_i + ZC_texcolor_offset + 13] = col_a.y;
		color_array[Zcolor_i + ZC_texcolor_offset + 14] = col_a.z;
		color_array[Zcolor_i + ZC_texcolor_offset + 15] = alpha;

		normal_array[Zvert_i + ZC_vertnorm_offset + 9] = na.x;
		normal_array[Zvert_i + ZC_vertnorm_offset + 10] = na.y;
		normal_array[Zvert_i + ZC_vertnorm_offset + 11] = na.z;

		scaleZCimage(ZCimage_index + 1, 1.0);
		planeZCimage(ZCimage_index + 1, { 0,0,-1 });

		ZCimage_index++;
		int ZCii = ZCimage_index;

		if (dyn_mode == false)
		{
			Zvert_i = Zvert_i + 12;
			Ztex_i = Ztex_i + 16;
			Zcolor_i = Zcolor_i + 16;
		}
		else
		{
			dyn_ZCimage_index = ZCimage_index;
			dyn_Zvert_i = Zvert_i + 12;
			dyn_Ztex_i = Ztex_i + 16;
			dyn_Zcolor_i = Zcolor_i + 16;

			Zvert_i = previous_Zvert_i;
			Ztex_i = previous_Ztex_i;
			Zcolor_i = previous_Zcolor_i;
			ZCimage_index = previous_ZCimage_index;
		}

		updateVBO = true;

		return ZCii;
	}

	int loadZCimage_patch_r(string filename, coords3d a, coords3d b, coords3d c, coords3d d, coords3d col_a, coords3d col_b, coords3d col_c, coords3d col_d, ipoint ta, ipoint tb, ipoint tc, ipoint td, coords3d na, coords3d nb, coords3d nc, coords3d nd, float fresnel, float envmap, float alpha)
	{
		multimap<string, int>::iterator it = atlas_map.find(filename);

		if (it == atlas_map.end())
		{
			if (failedtofind != filename)
			{
				if (global_suppressImages == false)
					cout << "Failed to find " << filename << " in atlas!\n";
				//failedtofind = filename;
				it = atlas_map.find("white0");
			}
			//it = atlas_map.find("black");
			//shutdown();
		}

		int previous_Zvert_i = 0;
		int previous_Ztex_i = 0;
		int previous_Zcolor_i = 0;
		int previous_ZCimage_index = 0;

		if (dyn_mode == true)
		{
			previous_Zvert_i = Zvert_i;
			previous_Ztex_i = Ztex_i;
			previous_Zcolor_i = Zcolor_i;

			previous_ZCimage_index = ZCimage_index;

			Zvert_i = dyn_Zvert_i;
			Ztex_i = dyn_Ztex_i;
			Zcolor_i = dyn_Zcolor_i;
			ZCimage_index = dyn_ZCimage_index;
		}

		ZCimage_tex_i[ZCimage_index] = it->second;

		// vertex 1

		vertex_array[Zvert_i + ZC_vertnorm_offset + 0] = a.x;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 1] = a.y;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 2] = a.z;

		texcoord_array[Ztex_i + ZC_texcolor_offset + 0] = twoDtoOneD({ atlas_x[ZCimage_tex_i[ZCimage_index]] + ta.x , atlas_y[ZCimage_tex_i[ZCimage_index]] + ta.y }, 2048);
		texcoord_array[Ztex_i + ZC_texcolor_offset + 1] = displacementA;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 2] = fresnel;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 3] = envmap;

		color_array[Zcolor_i + ZC_texcolor_offset + 0] = col_a.x;
		color_array[Zcolor_i + ZC_texcolor_offset + 1] = col_a.y;
		color_array[Zcolor_i + ZC_texcolor_offset + 2] = col_a.z;
		color_array[Zcolor_i + ZC_texcolor_offset + 3] = alpha;

		normal_array[Zvert_i + ZC_vertnorm_offset + 0] = na.x;
		normal_array[Zvert_i + ZC_vertnorm_offset + 1] = na.y;
		normal_array[Zvert_i + ZC_vertnorm_offset + 2] = na.z;


		// vertex 2

		vertex_array[Zvert_i + ZC_vertnorm_offset + 3] = b.x;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 4] = b.y;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 5] = b.z;

		texcoord_array[Ztex_i + ZC_texcolor_offset + 4] = twoDtoOneD({ atlas_x[ZCimage_tex_i[ZCimage_index]] + atlas_xsize[ZCimage_tex_i[ZCimage_index]] + tb.x , atlas_y[ZCimage_tex_i[ZCimage_index]] + tb.y }, 2048);
		texcoord_array[Ztex_i + ZC_texcolor_offset + 5] = displacementB;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 6] = fresnel;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 7] = envmap;

		color_array[Zcolor_i + ZC_texcolor_offset + 4] = col_b.x;
		color_array[Zcolor_i + ZC_texcolor_offset + 5] = col_b.y;
		color_array[Zcolor_i + ZC_texcolor_offset + 6] = col_b.z;
		color_array[Zcolor_i + ZC_texcolor_offset + 7] = alpha;

		normal_array[Zvert_i + ZC_vertnorm_offset + 3] = nb.x;
		normal_array[Zvert_i + ZC_vertnorm_offset + 4] = nb.y;
		normal_array[Zvert_i + ZC_vertnorm_offset + 5] = nb.z;


		// vertex 3

		vertex_array[Zvert_i + ZC_vertnorm_offset + 6] = c.x;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 7] = c.y;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 8] = c.z;

		texcoord_array[Ztex_i + ZC_texcolor_offset + 8] = twoDtoOneD({ atlas_x[ZCimage_tex_i[ZCimage_index]] + atlas_xsize[ZCimage_tex_i[ZCimage_index]] + tc.x , atlas_y[ZCimage_tex_i[ZCimage_index]] + atlas_ysize[ZCimage_tex_i[ZCimage_index]] + tc.y }, 2048);
		texcoord_array[Ztex_i + ZC_texcolor_offset + 9] = displacementC;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 10] = fresnel;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 11] = envmap;

		color_array[Zcolor_i + ZC_texcolor_offset + 8] = col_c.x;
		color_array[Zcolor_i + ZC_texcolor_offset + 9] = col_c.y;
		color_array[Zcolor_i + ZC_texcolor_offset + 10] = col_c.z;
		color_array[Zcolor_i + ZC_texcolor_offset + 11] = alpha;

		normal_array[Zvert_i + ZC_vertnorm_offset + 6] = nc.x;
		normal_array[Zvert_i + ZC_vertnorm_offset + 7] = nc.y;
		normal_array[Zvert_i + ZC_vertnorm_offset + 8] = nc.z;


		// vertex 4

		vertex_array[Zvert_i + ZC_vertnorm_offset + 9] = d.x;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 10] = d.y;
		vertex_array[Zvert_i + ZC_vertnorm_offset + 11] = d.z;

		texcoord_array[Ztex_i + ZC_texcolor_offset + 12] = twoDtoOneD({ atlas_x[ZCimage_tex_i[ZCimage_index]] + td.x , atlas_y[ZCimage_tex_i[ZCimage_index]] + atlas_ysize[ZCimage_tex_i[ZCimage_index]] + td.y }, 2048);
		texcoord_array[Ztex_i + ZC_texcolor_offset + 13] = displacementD;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 14] = fresnel;
		texcoord_array[Ztex_i + ZC_texcolor_offset + 15] = envmap;

		color_array[Zcolor_i + ZC_texcolor_offset + 12] = col_d.x;
		color_array[Zcolor_i + ZC_texcolor_offset + 13] = col_d.y;
		color_array[Zcolor_i + ZC_texcolor_offset + 14] = col_d.z;
		color_array[Zcolor_i + ZC_texcolor_offset + 15] = alpha;

		normal_array[Zvert_i + ZC_vertnorm_offset + 9] = nd.x;
		normal_array[Zvert_i + ZC_vertnorm_offset + 10] = nd.y;
		normal_array[Zvert_i + ZC_vertnorm_offset + 11] = nd.z;

		scaleZCimage(ZCimage_index + 1, 1.0);
		planeZCimage(ZCimage_index + 1, { 0,0,-1 });

		ZCimage_index++;
		int ZCii = ZCimage_index;

		if (dyn_mode == false)
		{
			Zvert_i = Zvert_i + 12;
			Ztex_i = Ztex_i + 16;
			Zcolor_i = Zcolor_i + 16;
		}
		else
		{
			dyn_ZCimage_index = ZCimage_index;
			dyn_Zvert_i = Zvert_i + 12;
			dyn_Ztex_i = Ztex_i + 16;
			dyn_Zcolor_i = Zcolor_i + 16;

			Zvert_i = previous_Zvert_i;
			Ztex_i = previous_Ztex_i;
			Zcolor_i = previous_Zcolor_i;
			ZCimage_index = previous_ZCimage_index;
		}

		updateVBO = true;

		return ZCii;
	}

	int loadHUDobject(string filename)
	{
		object_mutex.lock();

		bool firsttime = true;
		araFormat ara;

		ifstream arafile;
		arafile.open(filename, ios::binary);

		if (arafile.is_open())
		{
			string::size_type sz;

			arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			string version;
			version.resize(sz);
			arafile.read(&version[0], sz);

			arafile.read(reinterpret_cast<char*>(&ara.quad_count), sizeof(int));
			HUDobject_i[HUDobject_index].count = ara.quad_count;
			checkLimits(filename, 3, ara.quad_count);

			int winding = 0;
			arafile.read(reinterpret_cast<char*>(&winding), sizeof(int)); // Winding

			if (winding == 0)
				ara.m.reverse_winding = false;
			else if (winding == 1)
				ara.m.reverse_winding = true;

			arafile.read(reinterpret_cast<char*>(&ara.m.fresnel), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.envmap), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.alpha), sizeof(float));

			arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			ara.m.autotex_image.resize(sz);
			arafile.read(&ara.m.autotex_image[0], sz);

			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_zoom), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_mixratio), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_offset.x), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.autotex_offset.y), sizeof(float));

			arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			ara.m.multitex_image.resize(sz);
			arafile.read(&ara.m.multitex_image[0], sz);

			arafile.read(reinterpret_cast<char*>(&ara.m.multitex_zoom), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&ara.m.multitex_mixratio), sizeof(float));

			for (int i = 0; i < ara.quad_count; i++)
			{
				arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
				ara.q.tex_name.resize(sz);
				arafile.read(&ara.q.tex_name[0], sz);

				for (int j = 0; j < 4; j++)
				{
					arafile.read(reinterpret_cast<char*>(&ara.q.v[j].x), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.v[j].y), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.v[j].z), sizeof(float));
				}

				for (int j = 0; j < 4; j++)
				{
					arafile.read(reinterpret_cast<char*>(&ara.q.t[j].x), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.t[j].y), sizeof(float));
				}

				for (int j = 0; j < 4; j++)
				{
					arafile.read(reinterpret_cast<char*>(&ara.q.n[j].x), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.n[j].y), sizeof(float));
					arafile.read(reinterpret_cast<char*>(&ara.q.n[j].z), sizeof(float));
				}

				coords3d col = { 1.0f, 1.0f, 1.0f }; // We default vertex color to "white"

				if (ara.m.reverse_winding == false)
					ara.q.quad_handle = loadHUDimage_patch(ara.q.tex_name, ara.q.v[0], ara.q.v[1], ara.q.v[2], ara.q.v[3], col, col, col, col, ara.q.t[0], ara.q.t[1], ara.q.t[2], ara.q.t[3], ara.q.n[0], ara.q.n[1], ara.q.n[2], ara.q.n[3], ara.m.fresnel, ara.m.envmap, ara.m.alpha);

				else if (ara.m.reverse_winding == true)
					ara.q.quad_handle = loadHUDimage_patch_r(ara.q.tex_name, ara.q.v[0], ara.q.v[1], ara.q.v[2], ara.q.v[3], col, col, col, col, ara.q.t[0], ara.q.t[1], ara.q.t[2], ara.q.t[3], ara.q.n[0], ara.q.n[1], ara.q.n[2], ara.q.n[3], ara.m.fresnel, ara.m.envmap, ara.m.alpha);

				if (firsttime == true)
				{
					firsttime = false;
					HUDobject_i[HUDobject_index].startimage = ara.q.quad_handle;
				}

				if (ara.m.autotex_image != "")
				{
					autotexHUDimage(ara.q.quad_handle, ara.m.autotex_image, ara.m.autotex_zoom, ara.m.autotex_mixratio);
					scrollHUDImageAutoTexture(ara.q.quad_handle, ara.m.autotex_offset);
				}

				if (ara.m.multitex_image != "")
				{
					multitexHUDimage(ara.q.quad_handle, ara.m.multitex_image, ara.m.multitex_zoom, ara.m.multitex_mixratio);
				}
			}
		}
		else
			cout << filename << " HUDobject file not found!\n";

		arafile.close();

		HUDobject_index++;
		int oi = HUDobject_index;

		object_mutex.unlock();

		return oi;
	}

	void loadscn(string filename, coords3d position)
	{
		ifstream SCNfile;
		SCNfile.open(filename, ios::binary);

		string::size_type sz;

		SCNfile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
		string version;
		version.resize(sz);
		SCNfile.read(&version[0], sz);

		int SCNsize = 0;
		SCNfile.read(reinterpret_cast<char*>(&SCNsize), sizeof(int));

		for (int i = 0; i < SCNsize; i++)
		{
			string obj_file;
			SCNfile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			obj_file.resize(sz);
			SCNfile.read(&obj_file[0], sz);

			coords3d translate = { 0.0f,0.0f,0.0f };
			SCNfile.read(reinterpret_cast<char*>(&translate.x), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&translate.y), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&translate.z), sizeof(float));

			coords3d rotate = { 0.0f,0.0f,0.0f };
			SCNfile.read(reinterpret_cast<char*>(&rotate.x), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&rotate.y), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&rotate.z), sizeof(float));

			float scale = 0.0f;
			SCNfile.read(reinterpret_cast<char*>(&scale), sizeof(float));

			translate = translate + position;

			int o = loadobject("assets/" + obj_file);
			moveobject(o, translate);
			rotateobject(o, rotate);
			scaleobject(o, scale);
			glassobject(o, 1.0f); // This is a the one exception where we glass up, as we don't track handles for Scenes.
		}

		SCNfile.close();
	}

	void loadZCscn(string filename, coords3d position)
	{
		ifstream SCNfile;
		SCNfile.open(filename, ios::binary);

		string::size_type sz;

		SCNfile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
		string version;
		version.resize(sz);
		SCNfile.read(&version[0], sz);

		int SCNsize = 0;
		SCNfile.read(reinterpret_cast<char*>(&SCNsize), sizeof(int));

		for (int i = 0; i < SCNsize; i++)
		{
			string obj_file;
			SCNfile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			obj_file.resize(sz);
			SCNfile.read(&obj_file[0], sz);

			coords3d translate = { 0.0f,0.0f,0.0f };
			SCNfile.read(reinterpret_cast<char*>(&translate.x), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&translate.y), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&translate.z), sizeof(float));

			coords3d rotate = { 0.0f,0.0f,0.0f };
			SCNfile.read(reinterpret_cast<char*>(&rotate.x), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&rotate.y), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&rotate.z), sizeof(float));

			float scale = 0.0f;
			SCNfile.read(reinterpret_cast<char*>(&scale), sizeof(float));

			translate = translate + position;

			int o = loadZCobject("assets/" + obj_file);
			moveZCobject(o, translate);
			rotateZCobject(o, rotate);
			scaleZCobject(o, scale);
			glassZCobject(o, 1.0f); // This is a the one exception where we glass up, as we don't track handles for Scenes.
		}

		SCNfile.close();
	}

	void loadHUDscn(string filename, coords3d position)
	{
		ifstream SCNfile;
		SCNfile.open(filename, ios::binary);

		string::size_type sz;

		SCNfile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
		string version;
		version.resize(sz);
		SCNfile.read(&version[0], sz);

		int SCNsize = 0;
		SCNfile.read(reinterpret_cast<char*>(&SCNsize), sizeof(int));

		for (int i = 0; i < SCNsize; i++)
		{
			string obj_file;
			SCNfile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
			obj_file.resize(sz);
			SCNfile.read(&obj_file[0], sz);

			coords3d translate = { 0.0f,0.0f,0.0f };
			SCNfile.read(reinterpret_cast<char*>(&translate.x), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&translate.y), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&translate.z), sizeof(float));

			coords3d rotate = { 0.0f,0.0f,0.0f };
			SCNfile.read(reinterpret_cast<char*>(&rotate.x), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&rotate.y), sizeof(float));
			SCNfile.read(reinterpret_cast<char*>(&rotate.z), sizeof(float));

			float scale = 0.0f;
			SCNfile.read(reinterpret_cast<char*>(&scale), sizeof(float));

			translate = translate + position;

			int o = loadHUDobject("assets/" + obj_file);
			moveHUDobject(o, translate);
			rotateHUDobject(o, rotate);
			scaleHUDobject(o, scale);
			glassHUDobject(o, 1.0f); // This is a the one exception where we glass up, as we don't track handles for Scenes.
		}

		SCNfile.close();
	}

	int loadHUDobject_ice(string filename)
	{
		object_mutex.lock();

		bool firsttime = true;

		string manifold_name;
		string line;

		coords3d a, b, c, d;
		coords3d col_a, col_b, col_c, col_d;
		ipoint ta, tb, tc, td;
		coords3d na, nb, nc, nd;

		cout << "Load HUD object : " << filename << "\n";

		ifstream objfile("assets/" + filename);

		if (objfile.is_open())
		{
			std::getline(objfile, line); // Version check
			int fileversion = atoi(line.c_str());

			if (fileversion != 1104)
			{
				cout << "Invalid file version - " << filename << "\n";
				objfile.close();
				object_mutex.unlock();
				return -1;
			}

			std::getline(objfile, line); // Manifold count
			HUDobject_i[HUDobject_index].count = atoi(line.c_str());

			checkLimits(filename, 3, HUDobject_i[HUDobject_index].count);

			for (int i = 0; i < HUDobject_i[HUDobject_index].count; i++)
			{
				getline(objfile, manifold_name); // Object group ID (not used in production).
				getline(objfile, manifold_name); // Fast string to int.

				getline(objfile, line);
				a.x = stof(line.c_str());
				getline(objfile, line);
				a.y = stof(line.c_str());
				getline(objfile, line);
				a.z = stof(line.c_str());

				getline(objfile, line);
				b.x = stof(line.c_str());
				getline(objfile, line);
				b.y = stof(line.c_str());
				getline(objfile, line);
				b.z = stof(line.c_str());

				getline(objfile, line);
				c.x = stof(line.c_str());
				getline(objfile, line);
				c.y = stof(line.c_str());
				getline(objfile, line);
				c.z = stof(line.c_str());

				getline(objfile, line);
				d.x = stof(line.c_str());
				getline(objfile, line);
				d.y = stof(line.c_str());
				getline(objfile, line);
				d.z = stof(line.c_str());

				// Vertex Colors

				getline(objfile, line);
				col_a.x = stof(line.c_str());
				getline(objfile, line);
				col_a.y = stof(line.c_str());
				getline(objfile, line);
				col_a.z = stof(line.c_str());

				getline(objfile, line);
				col_b.x = stof(line.c_str());
				getline(objfile, line);
				col_b.y = stof(line.c_str());
				getline(objfile, line);
				col_b.z = stof(line.c_str());

				getline(objfile, line);
				col_c.x = stof(line.c_str());
				getline(objfile, line);
				col_c.y = stof(line.c_str());
				getline(objfile, line);
				col_c.z = stof(line.c_str());

				getline(objfile, line);
				col_d.x = stof(line.c_str());
				getline(objfile, line);
				col_d.y = stof(line.c_str());
				getline(objfile, line);
				col_d.z = stof(line.c_str());

				// Tex Coords

				getline(objfile, line);
				ta.x = atoi(line.c_str());
				getline(objfile, line);
				ta.y = atoi(line.c_str());

				getline(objfile, line);
				tb.x = atoi(line.c_str());
				getline(objfile, line);
				tb.y = atoi(line.c_str());

				getline(objfile, line);
				tc.x = atoi(line.c_str());
				getline(objfile, line);
				tc.y = atoi(line.c_str());

				getline(objfile, line);
				td.x = atoi(line.c_str());
				getline(objfile, line);
				td.y = atoi(line.c_str());

				// Normal Coords

				getline(objfile, line);
				na.x = stof(line.c_str());
				getline(objfile, line);
				na.y = stof(line.c_str());
				getline(objfile, line);
				na.z = stof(line.c_str());

				getline(objfile, line);
				nb.x = stof(line.c_str());
				getline(objfile, line);
				nb.y = stof(line.c_str());
				getline(objfile, line);
				nb.z = stof(line.c_str());

				getline(objfile, line);
				nc.x = stof(line.c_str());
				getline(objfile, line);
				nc.y = stof(line.c_str());
				getline(objfile, line);
				nc.z = stof(line.c_str());

				getline(objfile, line);
				nd.x = stof(line.c_str());
				getline(objfile, line);
				nd.y = stof(line.c_str());
				getline(objfile, line);
				nd.z = stof(line.c_str());

				getline(objfile, line);
				float alpha = stof(line.c_str());

				// .Reverse flag (Use forwarding winding command if true/1)

				getline(objfile, line);
				int r = atoi(line.c_str());

				getline(objfile, line);
				float fresnel = stof(line.c_str());
				getline(objfile, line);
				float envmap = stof(line.c_str());

				int ohandle = 0;

				if (r == 0)
					ohandle = loadHUDimage_patch(manifold_name, a, b, c, d, col_a, col_b, col_c, col_d, ta, tb, tc, td, na, nb, nc, nd, fresnel, envmap, alpha);
				else
					ohandle = loadHUDimage_patch_r(manifold_name, a, b, c, d, col_a, col_b, col_c, col_d, ta, tb, tc, td, na, nb, nc, nd, fresnel, envmap, alpha);

				if (firsttime == true)
				{
					firsttime = false;
					HUDobject_i[HUDobject_index].startimage = ohandle;
				}
			}
		}
		else
			cout << filename << " object file not found!\n";

		HUDobject_index++;

		int oi = HUDobject_index;

		object_mutex.unlock();

		return oi;
	}

	void moveHUDobject(int handle, coords3d pos)
	{
		for (int i = 0; i < HUDobject_i[handle - 1].count; i++)
			moveHUDimage(HUDobject_i[handle - 1].startimage + i, pos);
	}

	void rotateHUDobject(int handle, coords3d angle)
	{
		for (int i = 0; i < HUDobject_i[handle - 1].count; i++)
			rotateHUDimage(HUDobject_i[handle - 1].startimage + i, angle);
	}

	void scaleHUDobject(int handle, float scale)
	{
		for (int i = 0; i < HUDobject_i[handle - 1].count; i++)
			scaleHUDimage(HUDobject_i[handle - 1].startimage + i, scale);
	}

	void glassHUDobject(int handle, float opac)
	{
		for (int i = 0; i < HUDobject_i[handle - 1].count; i++)
			glassHUDimage(HUDobject_i[handle - 1].startimage + i, opac);
	}

	void moveVertex(int handle, int offset, coords3d a, coords3d b, coords3d c, coords3d d)
	{
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 0] = a.x; // CHANGE POS TO ABCD
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 1] = a.y;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 2] = a.z;

		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 4] = b.x;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 5] = b.y;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 6] = b.z;

		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 8] = c.x;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 9] = c.y;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 10] = c.z;

		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 12] = d.x;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 13] = d.y;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 14] = d.z;
	}

	void moveZCVertex(int handle, int offset, coords3d a, coords3d b, coords3d c, coords3d d)
	{
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 0] = a.x;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 1] = a.y;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 2] = a.z;

		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 4] = b.x;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 5] = b.y;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 6] = b.z;

		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 8] = c.x;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 9] = c.y;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 10] = c.z;

		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 12] = d.x;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 13] = d.y;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 14] = d.z;
	}

	void moveHUDVertex(int handle, int offset, coords3d a, coords3d b, coords3d c, coords3d d)
	{
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 0] = a.x;
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 1] = a.y;
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 2] = a.z;

		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 4] = b.x;
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 5] = b.y;
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 6] = b.z;

		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 8] = c.x;
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 9] = c.y;
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 10] = c.z;

		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 12] = d.x;
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 13] = d.y;
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 14] = d.z;
	}

	void scrollImageTexture(int handle, int offset, point scroll)
	{
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 0 + (texcolor_array_limit * 3)] = scroll.x; // TD.xy
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 1 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 4 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 5 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 8 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 9 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 12 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 13 + (texcolor_array_limit * 3)] = scroll.y;
	}

	void scrollZCImageTexture(int handle, int offset, point scroll)
	{
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 0 + (texcolor_array_limit * 3)] = scroll.x; // TD.xy
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 1 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 4 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 5 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 8 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 9 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 12 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 13 + (texcolor_array_limit * 3)] = scroll.y;
	}

	void scrollHUDImageTexture(int handle, int offset, point scroll)
	{
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 0 + (texcolor_array_limit * 3)] = scroll.x; // TD.xy
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 1 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 4 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 5 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 8 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 9 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 12 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((HUDobject_i[handle - 1].startimage + offset) - 1) * 16) + HUD_texcolor_offset + 13 + (texcolor_array_limit * 3)] = scroll.y;
	}

	void scrollImageAutoTexture(int handle, ipoint scroll)
	{
		att_PARASCAX[((handle - 1) * 16) + 3 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
		att_PARASCAX[((handle - 1) * 16) + 7 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
		att_PARASCAX[((handle - 1) * 16) + 11 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
		att_PARASCAX[((handle - 1) * 16) + 15 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
	}

	void scrollObjectAutoTexture(int handle, ipoint scroll)
	{
		for (int i = 0; i < object_i[handle - 1].count; i++)
			scrollImageAutoTexture(object_i[handle - 1].startimage + i, scroll);
	}

	void scrollZCImageAutoTexture(int handle, ipoint scroll)
	{
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 3 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 7 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 11 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 15 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
	}

	void scrollZCObjectAutoTexture(int handle, ipoint scroll)
	{
		for (int i = 0; i < ZCobject_i[handle - 1].count; i++)
			scrollZCImageAutoTexture(ZCobject_i[handle - 1].startimage + i, scroll);
	}

	void scrollHUDImageAutoTexture(int handle, ipoint scroll)
	{
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 3 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 7 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 11 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 15 + (texcolor_array_limit * 5)] = twoDtoOneD(scroll, 2048);
	}

	void scrollHUDObjectAutoTexture(int handle, ipoint scroll)
	{
		for (int i = 0; i < HUDobject_i[handle - 1].count; i++)
			scrollHUDImageAutoTexture(HUDobject_i[handle - 1].startimage + i, scroll);
	}

	void setDisplacementMap(string image) // See how it looks, maybe it needs texture trimmed -1 -1 +1 +1
	{
		ipoint pos;
		ipoint xysize;

		gettextureDimensions(image, pos, xysize);

		displacementA = twoDtoOneD({ pos.x , pos.y }, 2048);
		displacementB = twoDtoOneD({ int((pos.x + xysize.x) * 0.5), pos.y }, 2048);
		displacementC = twoDtoOneD({ int((pos.x + xysize.x) * 0.5), int((pos.y + xysize.y) * 0.5) }, 2048);
		displacementD = twoDtoOneD({ pos.x , int((pos.y + xysize.y) * 0.5) }, 2048);
	}

	void resetDisplacementMap()
	{
		displacementA = 0;
		displacementB = 0;
		displacementC = 0;
		displacementD = 0;
	}

	void scrollDisplacement(int handle, int offset, float mixratio, point scroll)
	{
		//att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 3 + texcolor_array_limit] = mixratio;
		//att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 7 + texcolor_array_limit] = mixratio;
		//att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 11 + texcolor_array_limit] = mixratio;
		//att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 15 + texcolor_array_limit] = mixratio;

		global_displacementMix_main = mixratio;

		att_PARASCAX[((object_i[handle - 1].startimage - 1) * 16) + 0 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(global_MTmix_main * 100.0f), int(mixratio * 100.0f) }, 2048);
		att_PARASCAX[((object_i[handle - 1].startimage - 1) * 16) + 4 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(global_MTmix_main * 100.0f), int(mixratio * 100.0f) }, 2048);
		att_PARASCAX[((object_i[handle - 1].startimage - 1) * 16) + 8 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(global_MTmix_main * 100.0f), int(mixratio * 100.0f) }, 2048);
		att_PARASCAX[((object_i[handle - 1].startimage - 1) * 16) + 12 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(global_MTmix_main * 100.0f), int(mixratio * 100.0f) }, 2048);

		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 2 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 3 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 6 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 7 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 10 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 11 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 14 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((object_i[handle - 1].startimage + offset) - 1) * 16) + 15 + (texcolor_array_limit * 3)] = scroll.y;
	}

	void scrollZCDisplacement(int handle, int offset, float mixratio, point scroll)
	{
		//att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 3 + texcolor_array_limit] = mixratio;
		//att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 7 + texcolor_array_limit] = mixratio;
		//att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 11 + texcolor_array_limit] = mixratio;
		//att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 15 + texcolor_array_limit] = mixratio;

		global_displacementMix_ZC = mixratio;

		att_PARASCAX[((ZCobject_i[handle - 1].startimage - 1) * 16) + ZC_texcolor_offset + 0 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(global_MTmix_ZC * 100.0f), int(mixratio * 100.0f) }, 2048);
		att_PARASCAX[((ZCobject_i[handle - 1].startimage - 1) * 16) + ZC_texcolor_offset + 4 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(global_MTmix_ZC * 100.0f), int(mixratio * 100.0f) }, 2048);
		att_PARASCAX[((ZCobject_i[handle - 1].startimage - 1) * 16) + ZC_texcolor_offset + 8 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(global_MTmix_ZC * 100.0f), int(mixratio * 100.0f) }, 2048);
		att_PARASCAX[((ZCobject_i[handle - 1].startimage - 1) * 16) + ZC_texcolor_offset + 12 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(global_MTmix_ZC * 100.0f), int(mixratio * 100.0f) }, 2048);

		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 2 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 3 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 6 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 7 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 10 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 11 + (texcolor_array_limit * 3)] = scroll.y;

		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 14 + (texcolor_array_limit * 3)] = scroll.x;
		att_PARASCAX[(((ZCobject_i[handle - 1].startimage + offset) - 1) * 16) + ZC_texcolor_offset + 15 + (texcolor_array_limit * 3)] = scroll.y;
	}

	void autoteximage(int handle, string image, float zoom, float mixratio)
	{
		ipoint pos;
		ipoint xysize;
		ipoint midpoint;

		gettextureDimensions(image, pos, xysize);

		midpoint = { (int)(pos.x + (xysize.x * 0.5f)), (int)(pos.y + (xysize.y * 0.5f)) };

		att_PARASCAX[((handle - 1) * 16) + 0 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048); // TX.xyz
		att_PARASCAX[((handle - 1) * 16) + 1 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + 2 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048); // Half-Extents

		att_PARASCAX[((handle - 1) * 16) + 4 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + 5 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + 6 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + 8 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + 9 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + 10 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + 12 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + 13 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + 14 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);
	}

	void autotexobject(int handle, string image, float zoom, float mixratio)
	{
		for (int i = 0; i < object_i[handle - 1].count; i++)
			autoteximage(object_i[handle - 1].startimage + i, image, zoom, mixratio);
	}

	void autotexZCimage(int handle, string image, float zoom, float mixratio)
	{
		ipoint pos;
		ipoint xysize;
		ipoint midpoint;

		gettextureDimensions(image, pos, xysize);

		midpoint = { (int)(pos.x + (xysize.x * 0.5f)), (int)(pos.y + (xysize.y * 0.5f)) };

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 0 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 1 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 2 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048); // Half-Extents

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 4 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 5 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 6 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 8 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 9 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 10 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 12 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 13 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 14 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);
	}

	void autotexZCobject(int handle, string image, float zoom, float mixratio)
	{
		for (int i = 0; i < ZCobject_i[handle - 1].count; i++)
			autotexZCimage(ZCobject_i[handle - 1].startimage + i, image, zoom, mixratio);
	}

	void autotexHUDimage(int handle, string image, float zoom, float mixratio)
	{
		ipoint pos;
		ipoint xysize;
		ipoint midpoint;

		gettextureDimensions(image, pos, xysize);

		midpoint = { (int)(pos.x + (xysize.x * 0.5f)), (int)(pos.y + (xysize.y * 0.5f)) };

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 0 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 1 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 2 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048); // Half-Extents

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 4 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 5 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 6 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 8 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 9 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 10 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 12 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(mixratio * 100.0f), int(zoom) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 13 + (texcolor_array_limit * 5)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 14 + (texcolor_array_limit * 5)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);
	}

	void autotexHUDobject(int handle, string image, float zoom, float mixratio)
	{
		for (int i = 0; i < HUDobject_i[handle - 1].count; i++)
			autotexHUDimage(HUDobject_i[handle - 1].startimage + i, image, zoom, mixratio);
	}

	void multiteximage (int handle, string image, float zoom, float mixratio)
	{
		ipoint pos;
		ipoint xysize;
		ipoint midpoint;

		gettextureDimensions(image, pos, xysize);

		midpoint = { (int)(pos.x + (xysize.x * 0.5f)), (int)(pos.y + (xysize.y * 0.5f)) };

		global_MTmix_main = mixratio;

		att_PARASCAX[((handle - 1) * 16) + 0 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_main * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + 1 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + 2 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + 3 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048); // Half-Extents

		att_PARASCAX[((handle - 1) * 16) + 4 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_main * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + 5 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + 6 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + 7 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + 8 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_main * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + 9 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + 10 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + 11 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + 12 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_main * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + 13 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + 14 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + 15 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);
	}

	void multitexobject(int handle, string image, float zoom, float mixratio)
	{
		for (int i = 0; i < object_i[handle - 1].count; i++)
			multiteximage(object_i[handle - 1].startimage + i, image, zoom, mixratio);
	}

	void multitexZCimage(int handle, string image, float zoom, float mixratio)
	{
		ipoint pos;
		ipoint xysize;
		ipoint midpoint;

		gettextureDimensions(image, pos, xysize);

		midpoint = { (int)(pos.x + (xysize.x * 0.5f)), (int)(pos.y + (xysize.y * 0.5f)) };

		global_MTmix_ZC = mixratio;

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 0 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_ZC * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 1 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 2 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 3 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 4 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_ZC * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 5 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 6 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 7 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 8 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_ZC * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 9 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 10 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 11 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 12 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_ZC * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 13 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 14 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 15 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);
	}

	void multitexZCobject(int handle, string image, float zoom, float mixratio)
	{
		for (int i = 0; i < ZCobject_i[handle - 1].count; i++)
			multitexZCimage(ZCobject_i[handle - 1].startimage + i, image, zoom, mixratio);
	}

	void multitexHUDimage(int handle, string image, float zoom, float mixratio)
	{
		ipoint pos;
		ipoint xysize;
		ipoint midpoint;

		gettextureDimensions(image, pos, xysize);

		midpoint = { (int)(pos.x + (xysize.x * 0.5f)), (int)(pos.y + (xysize.y * 0.5f)) };

		global_MTmix_HUD = mixratio;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 0 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_HUD * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 1 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 2 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 3 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 4 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_HUD * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 5 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 6 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 7 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 8 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_HUD * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 9 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 10 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 11 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 12 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(mixratio * 100.0f), int(global_displacementMix_HUD * 100.0f) }, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 13 + (texcolor_array_limit * 4)] = zoom;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 14 + (texcolor_array_limit * 4)] = twoDtoOneD(midpoint, 2048);
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 15 + (texcolor_array_limit * 4)] = twoDtoOneD({ int(xysize.x * 0.5f), int(xysize.y * 0.5f) }, 2048);
	}

	void multitexHUDobject(int handle, string image, float zoom, float mixratio)
	{
		for (int i = 0; i < HUDobject_i[handle - 1].count; i++)
			multitexHUDimage(HUDobject_i[handle - 1].startimage + i, image, zoom, mixratio);
	}

	int loadHUDimage_patch(string filename, coords3d a, coords3d b, coords3d c, coords3d d, coords3d col_a, coords3d col_b, coords3d col_c, coords3d col_d, ipoint ta, ipoint tb, ipoint tc, ipoint td, coords3d na, coords3d nb, coords3d nc, coords3d nd, float fresnel, float envmap, float alpha)
	{
		multimap<string, int>::iterator it = atlas_map.find(filename);

		if (it == atlas_map.end())
		{
			if (failedtofind != filename)
			{
				if (global_suppressImages == false)
					cout << "Failed to find " << filename << " in atlas!\n";
				//failedtofind = filename;
				it = atlas_map.find("white0");
			}
			//it = atlas_map.find("black");
			//shutdown();
		}

		int previous_Hvert_i = 0;
		int previous_Htex_i = 0;
		int previous_Hcolor_i = 0;
		int previous_HUDimage_index = 0;

		if (dyn_mode == true)
		{
			previous_Hvert_i = Hvert_i;
			previous_Htex_i = Htex_i;
			previous_Hcolor_i = Hcolor_i;

			previous_HUDimage_index = HUDimage_index;

			Hvert_i = dyn_Hvert_i;
			Htex_i = dyn_Htex_i;
			Hcolor_i = dyn_Hcolor_i;
			HUDimage_index = dyn_HUDimage_index;
		}

		HUDimage_tex_i[HUDimage_index] = it->second;

		HUDimage_size[HUDimage_index].x = atlas_xsize[HUDimage_tex_i[HUDimage_index]];
		HUDimage_size[HUDimage_index].y = atlas_ysize[HUDimage_tex_i[HUDimage_index]];

		// vertex 1

		vertex_array[Hvert_i + HUD_vertnorm_offset + 0] = d.x * 100.0f; // 3 * 4 = 12 * 4500 = 54,000
		vertex_array[Hvert_i + HUD_vertnorm_offset + 1] = d.y * 100.0f; //         12 * 5500 = 66,000
		vertex_array[Hvert_i + HUD_vertnorm_offset + 2] = d.z * 100.0f;

		texcoord_array[Htex_i + HUD_texcolor_offset + 0] = twoDtoOneD({ atlas_x[HUDimage_tex_i[HUDimage_index]] + td.x , atlas_y[HUDimage_tex_i[HUDimage_index]] + atlas_ysize[HUDimage_tex_i[HUDimage_index]] + td.y }, 2048);
		texcoord_array[Htex_i + HUD_texcolor_offset + 1] = displacementD;
		texcoord_array[Htex_i + HUD_texcolor_offset + 2] = fresnel;
		texcoord_array[Htex_i + HUD_texcolor_offset + 3] = envmap;

		color_array[Hcolor_i + HUD_texcolor_offset + 0] = col_d.x;
		color_array[Hcolor_i + HUD_texcolor_offset + 1] = col_d.y;
		color_array[Hcolor_i + HUD_texcolor_offset + 2] = col_d.z;
		color_array[Hcolor_i + HUD_texcolor_offset + 3] = alpha;

		normal_array[Zvert_i + HUD_vertnorm_offset + 0] = nd.x;
		normal_array[Zvert_i + HUD_vertnorm_offset + 1] = nd.y;
		normal_array[Zvert_i + HUD_vertnorm_offset + 2] = nd.z;


		// vertex 2

		vertex_array[Hvert_i + HUD_vertnorm_offset + 3] = c.x * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 4] = c.y * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 5] = c.z * 100.0f;

		texcoord_array[Htex_i + HUD_texcolor_offset + 4] = twoDtoOneD({ atlas_x[HUDimage_tex_i[HUDimage_index]] + atlas_xsize[HUDimage_tex_i[HUDimage_index]] + tc.x , atlas_y[HUDimage_tex_i[HUDimage_index]] + atlas_ysize[HUDimage_tex_i[HUDimage_index]] + tc.y }, 2048);
		texcoord_array[Htex_i + HUD_texcolor_offset + 5] = displacementC;
		texcoord_array[Htex_i + HUD_texcolor_offset + 6] = fresnel;
		texcoord_array[Htex_i + HUD_texcolor_offset + 7] = envmap;

		color_array[Hcolor_i + HUD_texcolor_offset + 4] = col_c.x;
		color_array[Hcolor_i + HUD_texcolor_offset + 5] = col_c.y;
		color_array[Hcolor_i + HUD_texcolor_offset + 6] = col_c.z;
		color_array[Hcolor_i + HUD_texcolor_offset + 7] = alpha;

		normal_array[Hvert_i + HUD_vertnorm_offset + 3] = nc.x;
		normal_array[Hvert_i + HUD_vertnorm_offset + 4] = nc.y;
		normal_array[Hvert_i + HUD_vertnorm_offset + 5] = nc.z;


		// vertex 3

		vertex_array[Hvert_i + HUD_vertnorm_offset + 6] = b.x * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 7] = b.y * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 8] = b.z * 100.0f;

		texcoord_array[Htex_i + HUD_texcolor_offset + 8] = twoDtoOneD({ atlas_x[HUDimage_tex_i[HUDimage_index]] + atlas_xsize[HUDimage_tex_i[HUDimage_index]] + tb.x , atlas_y[HUDimage_tex_i[HUDimage_index]] + tb.y }, 2048);
		texcoord_array[Htex_i + HUD_texcolor_offset + 9] = displacementB;
		texcoord_array[Htex_i + HUD_texcolor_offset + 10] = fresnel;
		texcoord_array[Htex_i + HUD_texcolor_offset + 11] = envmap;

		color_array[Hcolor_i + HUD_texcolor_offset + 8] = col_b.x;
		color_array[Hcolor_i + HUD_texcolor_offset + 9] = col_b.y;
		color_array[Hcolor_i + HUD_texcolor_offset + 10] = col_b.z;
		color_array[Hcolor_i + HUD_texcolor_offset + 11] = alpha;

		normal_array[Hvert_i + HUD_vertnorm_offset + 6] = nb.x;
		normal_array[Hvert_i + HUD_vertnorm_offset + 7] = nb.y;
		normal_array[Hvert_i + HUD_vertnorm_offset + 8] = nb.z;


		// vertex 4

		vertex_array[Hvert_i + HUD_vertnorm_offset + 9] = a.x * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 10] = a.y * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 11] = a.z * 100.0f;

		texcoord_array[Htex_i + HUD_texcolor_offset + 12] = twoDtoOneD({ atlas_x[HUDimage_tex_i[HUDimage_index]] + ta.x , atlas_y[HUDimage_tex_i[HUDimage_index]] + ta.y }, 2048);
		texcoord_array[Htex_i + HUD_texcolor_offset + 13] = displacementA;
		texcoord_array[Htex_i + HUD_texcolor_offset + 14] = fresnel;
		texcoord_array[Htex_i + HUD_texcolor_offset + 15] = envmap;

		color_array[Hcolor_i + HUD_texcolor_offset + 12] = col_a.x;
		color_array[Hcolor_i + HUD_texcolor_offset + 13] = col_a.y;
		color_array[Hcolor_i + HUD_texcolor_offset + 14] = col_a.z;
		color_array[Hcolor_i + HUD_texcolor_offset + 15] = alpha;

		normal_array[Hvert_i + HUD_vertnorm_offset + 9] = na.x;
		normal_array[Hvert_i + HUD_vertnorm_offset + 10] = na.y;
		normal_array[Hvert_i + HUD_vertnorm_offset + 11] = na.z;

		scaleHUDimage(HUDimage_index + 1, 1.0);
		planeHUDimage(HUDimage_index + 1, { 0,0,-1 });

		HUDimage_index++;
		int HUDii = HUDimage_index;

		if (dyn_mode == false)
		{
			Hvert_i = Hvert_i + 12;
			Htex_i = Htex_i + 16;
			Hcolor_i = Hcolor_i + 16;
		}
		else
		{
			dyn_HUDimage_index = HUDimage_index;
			dyn_Hvert_i = Hvert_i + 12;
			dyn_Htex_i = Htex_i + 16;
			dyn_Hcolor_i = Hcolor_i + 16;

			Hvert_i = previous_Hvert_i;
			Htex_i = previous_Htex_i;
			Hcolor_i = previous_Hcolor_i;
			HUDimage_index = previous_HUDimage_index;
		}

		updateVBO = true;

		return HUDii;
	}

	int loadHUDimage_patch_r(string filename, coords3d a, coords3d b, coords3d c, coords3d d, coords3d col_a, coords3d col_b, coords3d col_c, coords3d col_d, ipoint ta, ipoint tb, ipoint tc, ipoint td, coords3d na, coords3d nb, coords3d nc, coords3d nd, float fresnel, float envmap, float alpha)
	{
		multimap<string, int>::iterator it = atlas_map.find(filename);

		if (it == atlas_map.end())
		{
			if (failedtofind != filename)
			{
				if (global_suppressImages == false)
					cout << "Failed to find " << filename << " in atlas!\n";
				//failedtofind = filename;
				it = atlas_map.find("white0");
			}
			//it = atlas_map.find("black");
			//shutdown();
		}

		int previous_Hvert_i = 0;
		int previous_Htex_i = 0;
		int previous_Hcolor_i = 0;
		int previous_HUDimage_index = 0;

		if (dyn_mode == true)
		{
			previous_Hvert_i = Hvert_i;
			previous_Htex_i = Htex_i;
			previous_Hcolor_i = Hcolor_i;

			previous_HUDimage_index = HUDimage_index;

			Hvert_i = dyn_Hvert_i;
			Htex_i = dyn_Htex_i;
			Hcolor_i = dyn_Hcolor_i;
			HUDimage_index = dyn_HUDimage_index;
		}

		HUDimage_tex_i[HUDimage_index] = it->second;

		HUDimage_size[HUDimage_index].x = atlas_xsize[HUDimage_tex_i[HUDimage_index]];
		HUDimage_size[HUDimage_index].y = atlas_ysize[HUDimage_tex_i[HUDimage_index]];

		// vertex 1

		vertex_array[Hvert_i + HUD_vertnorm_offset + 0] = a.x * 100.0f; // doesn't work
		vertex_array[Hvert_i + HUD_vertnorm_offset + 1] = a.y * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 2] = a.z * 100.0f;

		texcoord_array[Htex_i + HUD_texcolor_offset + 0] = twoDtoOneD({ atlas_x[HUDimage_tex_i[HUDimage_index]] + ta.x , atlas_y[HUDimage_tex_i[HUDimage_index]] + ta.y }, 2048);
		texcoord_array[Htex_i + HUD_texcolor_offset + 1] = displacementA;
		texcoord_array[Htex_i + HUD_texcolor_offset + 2] = fresnel;
		texcoord_array[Htex_i + HUD_texcolor_offset + 3] = envmap;

		color_array[Hcolor_i + HUD_texcolor_offset + 0] = col_a.x;
		color_array[Hcolor_i + HUD_texcolor_offset + 1] = col_a.y;
		color_array[Hcolor_i + HUD_texcolor_offset + 2] = col_a.z;
		color_array[Hcolor_i + HUD_texcolor_offset + 3] = alpha;

		normal_array[Hvert_i + HUD_vertnorm_offset + 0] = na.x;
		normal_array[Hvert_i + HUD_vertnorm_offset + 1] = na.y;
		normal_array[Hvert_i + HUD_vertnorm_offset + 2] = na.z;


		// vertex 2

		vertex_array[Hvert_i + HUD_vertnorm_offset + 3] = b.x * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 4] = b.y * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 5] = b.z * 100.0f;

		texcoord_array[Htex_i + HUD_texcolor_offset + 4] = twoDtoOneD({ atlas_x[HUDimage_tex_i[HUDimage_index]] + atlas_xsize[HUDimage_tex_i[HUDimage_index]] + tb.x , atlas_y[HUDimage_tex_i[HUDimage_index]] + tb.y }, 2048);
		texcoord_array[Htex_i + HUD_texcolor_offset + 5] = displacementB;
		texcoord_array[Htex_i + HUD_texcolor_offset + 6] = fresnel;
		texcoord_array[Htex_i + HUD_texcolor_offset + 7] = envmap;

		color_array[Hcolor_i + HUD_texcolor_offset + 4] = col_b.x;
		color_array[Hcolor_i + HUD_texcolor_offset + 5] = col_b.y;
		color_array[Hcolor_i + HUD_texcolor_offset + 6] = col_b.z;
		color_array[Hcolor_i + HUD_texcolor_offset + 7] = alpha;

		normal_array[Hvert_i + HUD_vertnorm_offset + 3] = nb.x;
		normal_array[Hvert_i + HUD_vertnorm_offset + 4] = nb.y;
		normal_array[Hvert_i + HUD_vertnorm_offset + 5] = nb.z;


		// vertex 3

		vertex_array[Hvert_i + HUD_vertnorm_offset + 6] = c.x * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 7] = c.y * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 8] = c.z * 100.0f;

		texcoord_array[Htex_i + HUD_texcolor_offset + 8] = twoDtoOneD({ atlas_x[HUDimage_tex_i[HUDimage_index]] + atlas_xsize[HUDimage_tex_i[HUDimage_index]] + tc.x , atlas_y[HUDimage_tex_i[HUDimage_index]] + atlas_ysize[HUDimage_tex_i[HUDimage_index]] + tc.y }, 2048);
		texcoord_array[Htex_i + HUD_texcolor_offset + 9] = displacementC;
		texcoord_array[Htex_i + HUD_texcolor_offset + 10] = fresnel;
		texcoord_array[Htex_i + HUD_texcolor_offset + 11] = envmap;

		color_array[Hcolor_i + HUD_texcolor_offset + 8] = col_c.x;
		color_array[Hcolor_i + HUD_texcolor_offset + 9] = col_c.y;
		color_array[Hcolor_i + HUD_texcolor_offset + 10] = col_c.z;
		color_array[Hcolor_i + HUD_texcolor_offset + 11] = alpha;

		normal_array[Hvert_i + HUD_vertnorm_offset + 6] = nc.x;
		normal_array[Hvert_i + HUD_vertnorm_offset + 7] = nc.y;
		normal_array[Hvert_i + HUD_vertnorm_offset + 8] = nc.z;


		// vertex 4

		vertex_array[Hvert_i + HUD_vertnorm_offset + 9] = d.x * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 10] = d.y * 100.0f;
		vertex_array[Hvert_i + HUD_vertnorm_offset + 11] = d.z * 100.0f;

		texcoord_array[Htex_i + HUD_texcolor_offset + 12] = twoDtoOneD({ atlas_x[HUDimage_tex_i[HUDimage_index]] + td.x , atlas_y[HUDimage_tex_i[HUDimage_index]] + atlas_ysize[HUDimage_tex_i[HUDimage_index]] + td.y }, 2048);
		texcoord_array[Htex_i + HUD_texcolor_offset + 13] = displacementD;
		texcoord_array[Htex_i + HUD_texcolor_offset + 14] = fresnel;
		texcoord_array[Htex_i + HUD_texcolor_offset + 15] = envmap;

		color_array[Hcolor_i + HUD_texcolor_offset + 12] = col_d.x;
		color_array[Hcolor_i + HUD_texcolor_offset + 13] = col_d.y;
		color_array[Hcolor_i + HUD_texcolor_offset + 14] = col_d.z;
		color_array[Hcolor_i + HUD_texcolor_offset + 15] = alpha;

		normal_array[Hvert_i + HUD_vertnorm_offset + 9] = nd.x;
		normal_array[Hvert_i + HUD_vertnorm_offset + 10] = nd.y;
		normal_array[Hvert_i + HUD_vertnorm_offset + 11] = nd.z;

		scaleHUDimage(HUDimage_index + 1, 1.0);
		planeHUDimage(HUDimage_index + 1, { 0,0,-1 });

		HUDimage_index++;
		int HUDii = HUDimage_index;

		if (dyn_mode == false)
		{
			Hvert_i = Hvert_i + 12;
			Htex_i = Htex_i + 16;
			Hcolor_i = Hcolor_i + 16;
		}
		else
		{
			dyn_HUDimage_index = HUDimage_index;
			dyn_Hvert_i = Hvert_i + 12;
			dyn_Htex_i = Htex_i + 16;
			dyn_Hcolor_i = Hcolor_i + 16;

			Hvert_i = previous_Hvert_i;
			Htex_i = previous_Htex_i;
			Hcolor_i = previous_Hcolor_i;
			HUDimage_index = previous_HUDimage_index;
		}

		updateVBO = true;

		return HUDii;
	}

	void updateHUDimage_patch(int handle, string filename, coords3d a, coords3d b, coords3d c, coords3d d, coords3d col_a, coords3d col_b, coords3d col_c, coords3d col_d, ipoint ta, ipoint tb, ipoint tc, ipoint td, float fresnel, float envmap, float alpha)
	{
		multimap<string, int>::iterator it = atlas_map.find(filename);

		if (it == atlas_map.end())
		{
			if (failedtofind != filename)
			{
				if (global_suppressImages == false)
					cout << "Failed to find " << filename << " in atlas!\n";
				//failedtofind = filename;
				it = atlas_map.find("white0");
			}
			//it = atlas_map.find("black");
			//shutdown();
		}

		HUDimage_tex_i[handle - 1] = it->second;

		int _Hvert_i = HUD_vertnorm_offset + ((handle - 1) * 12);
		int _Htex_i = HUD_texcolor_offset + ((handle - 1) * 16);
		int _Hcolor_i = HUD_texcolor_offset + ((handle - 1) * 16);

		// vertex 1

		vertex_array[_Hvert_i + 0] = d.x * 100.0f;
		vertex_array[_Hvert_i + 1] = d.y * 100.0f;
		vertex_array[_Hvert_i + 2] = d.z * 100.0f;

		texcoord_array[_Htex_i + 0] = twoDtoOneD({ atlas_x[HUDimage_tex_i[handle - 1]] + td.x , (atlas_y[HUDimage_tex_i[handle - 1]] + atlas_ysize[HUDimage_tex_i[handle - 1]]) + td.y }, 2048);
		texcoord_array[_Htex_i + 1] = displacementD;
		texcoord_array[_Htex_i + 2] = fresnel;
		texcoord_array[_Htex_i + 3] = envmap;

		color_array[_Hcolor_i + 0] = col_d.x;
		color_array[_Hcolor_i + 1] = col_d.y;
		color_array[_Hcolor_i + 2] = col_d.z;
		color_array[_Hcolor_i + 3] = alpha;


		// vertex 2

		vertex_array[_Hvert_i + 3] = c.x * 100.0f;
		vertex_array[_Hvert_i + 4] = c.y * 100.0f;
		vertex_array[_Hvert_i + 5] = c.z * 100.0f;

		texcoord_array[_Htex_i + 4] = twoDtoOneD({ (atlas_x[HUDimage_tex_i[handle - 1]] + atlas_xsize[HUDimage_tex_i[handle - 1]]) + tc.x , (atlas_y[HUDimage_tex_i[handle - 1]] + atlas_ysize[HUDimage_tex_i[handle - 1]]) + tc.y }, 2048);
		texcoord_array[_Htex_i + 5] = displacementC;
		texcoord_array[_Htex_i + 6] = fresnel;
		texcoord_array[_Htex_i + 7] = envmap;

		color_array[_Hcolor_i + 4] = col_c.x;
		color_array[_Hcolor_i + 5] = col_c.y;
		color_array[_Hcolor_i + 6] = col_c.z;
		color_array[_Hcolor_i + 7] = alpha;


		// vertex 3

		vertex_array[_Hvert_i + 6] = b.x * 100.0f;
		vertex_array[_Hvert_i + 7] = b.y * 100.0f;
		vertex_array[_Hvert_i + 8] = b.z * 100.0f;

		texcoord_array[_Htex_i + 8] = twoDtoOneD({ (atlas_x[HUDimage_tex_i[handle - 1]] + atlas_xsize[HUDimage_tex_i[handle - 1]]) + tb.x , atlas_y[HUDimage_tex_i[handle - 1]] + tb.y }, 2048);
		texcoord_array[_Htex_i + 9] = displacementB;
		texcoord_array[_Htex_i + 10] = fresnel;
		texcoord_array[_Htex_i + 11] = envmap;

		color_array[_Hcolor_i + 8] = col_b.x;
		color_array[_Hcolor_i + 9] = col_b.y;
		color_array[_Hcolor_i + 10] = col_b.z;
		color_array[_Hcolor_i + 11] = alpha;


		// vertex 4

		vertex_array[_Hvert_i + 9] = a.x * 100.0f;
		vertex_array[_Hvert_i + 10] = a.y * 100.0f;
		vertex_array[_Hvert_i + 11] = a.z * 100.0f;

		twoDtoOneD({ atlas_x[HUDimage_tex_i[handle - 1]] + ta.x , atlas_y[HUDimage_tex_i[handle - 1]] + ta.y }, 2048);

		texcoord_array[_Htex_i + 12] = twoDtoOneD({ atlas_x[HUDimage_tex_i[handle - 1]] + ta.x , atlas_y[HUDimage_tex_i[handle - 1]] + ta.y }, 2048);
		texcoord_array[_Htex_i + 13] = displacementA;
		texcoord_array[_Htex_i + 14] = fresnel;
		texcoord_array[_Htex_i + 15] = envmap;

		color_array[_Hcolor_i + 12] = col_a.x;
		color_array[_Hcolor_i + 13] = col_a.y;
		color_array[_Hcolor_i + 14] = col_a.z;
		color_array[_Hcolor_i + 15] = alpha;

		scaleHUDimage(handle, 1.0);
		planeHUDimage(handle, { 0,0,-1 });

		updateVBO = true;
	}

	void moveimage(int handle, coords3d pos)
	{
		att_PARASCAX[((handle - 1) * 16) + 0] = pos.x;
		att_PARASCAX[((handle - 1) * 16) + 1] = pos.y;
		att_PARASCAX[((handle - 1) * 16) + 2] = pos.z;

		att_PARASCAX[((handle - 1) * 16) + 4] = pos.x;
		att_PARASCAX[((handle - 1) * 16) + 5] = pos.y;
		att_PARASCAX[((handle - 1) * 16) + 6] = pos.z;

		att_PARASCAX[((handle - 1) * 16) + 8] = pos.x;
		att_PARASCAX[((handle - 1) * 16) + 9] = pos.y;
		att_PARASCAX[((handle - 1) * 16) + 10] = pos.z;

		att_PARASCAX[((handle - 1) * 16) + 12] = pos.x;
		att_PARASCAX[((handle - 1) * 16) + 13] = pos.y;
		att_PARASCAX[((handle - 1) * 16) + 14] = pos.z;
	}

	void moveZCimage(int handle, coords3d pos) // NOW NEED TO UPDATE THE OFFSETS. -- THIS IS AN OLD TASK, - PLEASE CHECK THIS CALC MAKES SENSE AND IT WAS DONE.
	{
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 0] = pos.x; // 2500 quads 5000 triangles * 3 = 15,000 verts x 4 attrib floats = 60,000
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 1] = pos.y; // 4500		9000 tri * 3 = 27,000 verts x 4 attrib floats = 108,000
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 2] = pos.z; // 4500        n/a   * 4 = 18,000 verts x 4 attrib float = 72,000   

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 4] = pos.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 5] = pos.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 6] = pos.z;

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 8] = pos.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 9] = pos.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 10] = pos.z;

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 12] = pos.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 13] = pos.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 14] = pos.z;
	}

	void moveHUDimage(int handle, coords3d pos)
	{
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 0] = pos.x * 100.0f;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 1] = pos.y * 100.0f;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 2] = pos.z * 100.0f;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 4] = pos.x * 100.0f;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 5] = pos.y * 100.0f;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 6] = pos.z * 100.0f;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 8] = pos.x * 100.0f;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 9] = pos.y * 100.0f;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 10] = pos.z * 100.0f;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 12] = pos.x * 100.0f;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 13] = pos.y * 100.0f;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 14] = pos.z * 100.0f;
	}

	void scaleimage(int handle, float scale)
	{
		att_PARASCAX[((handle - 1) * 16) + 0 + (texcolor_array_limit * 2)] = scale;
		att_PARASCAX[((handle - 1) * 16) + 4 + (texcolor_array_limit * 2)] = scale;
		att_PARASCAX[((handle - 1) * 16) + 8 + (texcolor_array_limit * 2)] = scale;
		att_PARASCAX[((handle - 1) * 16) + 12 + (texcolor_array_limit * 2)] = scale;
	}

	void scaleZCimage(int handle, float scale)
	{
		att_PARASCAX[((handle - 1) * 16) + (ZC_texcolor_offset)+0 + (texcolor_array_limit * 2)] = scale;
		att_PARASCAX[((handle - 1) * 16) + (ZC_texcolor_offset)+4 + (texcolor_array_limit * 2)] = scale;
		att_PARASCAX[((handle - 1) * 16) + (ZC_texcolor_offset)+8 + (texcolor_array_limit * 2)] = scale;
		att_PARASCAX[((handle - 1) * 16) + (ZC_texcolor_offset)+12 + (texcolor_array_limit * 2)] = scale;
	}

	void scaleHUDimage(int handle, float scale)
	{
		att_PARASCAX[((handle - 1) * 16) + (HUD_texcolor_offset)+0 + (texcolor_array_limit * 2)] = scale;
		att_PARASCAX[((handle - 1) * 16) + (HUD_texcolor_offset)+4 + (texcolor_array_limit * 2)] = scale;
		att_PARASCAX[((handle - 1) * 16) + (HUD_texcolor_offset)+8 + (texcolor_array_limit * 2)] = scale;
		att_PARASCAX[((handle - 1) * 16) + (HUD_texcolor_offset)+12 + (texcolor_array_limit * 2)] = scale;
	}

	void rotateimage(int handle, coords3d angle) // Convert it from an Euler-angle to our native Quaternion.
	{
		quat q;

		// Basically we create 3 Quaternions, one for pitch, one for yaw, one for roll // and multiply those together. // the calculation below does the same, just shorter

		float p = angle.y * (M_PI / 180.0) / 2.0f; // Maybe expects "angle" to be a radian..
		float y = angle.z * (M_PI / 180.0) / 2.0f;
		float r = angle.x * (M_PI / 180.0) / 2.0f;

		float sinp = sin(p);
		float siny = sin(y);
		float sinr = sin(r);
		float cosp = cos(p);
		float cosy = cos(y);
		float cosr = cos(r);

		q.x = sinr * cosp * cosy - cosr * sinp * siny;
		q.y = cosr * sinp * cosy + sinr * cosp * siny;
		q.z = cosr * cosp * siny - sinr * sinp * cosy;
		q.s = cosr * cosp * cosy + sinr * sinp * siny;

		q = Qnormalize(q);

		att_PARASCAX[((handle - 1) * 16) + 0 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + 1 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + 2 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + 3 + texcolor_array_limit] = q.s; // We use MX.w, because it was spare.

		att_PARASCAX[((handle - 1) * 16) + 4 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + 5 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + 6 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + 7 + texcolor_array_limit] = q.s;

		att_PARASCAX[((handle - 1) * 16) + 8 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + 9 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + 10 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + 11 + texcolor_array_limit] = q.s;

		att_PARASCAX[((handle - 1) * 16) + 12 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + 13 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + 14 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + 15 + texcolor_array_limit] = q.s;
	}

	void Qrotateimage(int handle, quat angle)
	{
		att_PARASCAX[((handle - 1) * 16) + 0 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + 1 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + 2 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + 3 + texcolor_array_limit] = angle.s; // We use MX.w, because it was spare.

		att_PARASCAX[((handle - 1) * 16) + 4 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + 5 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + 6 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + 7 + texcolor_array_limit] = angle.s;

		att_PARASCAX[((handle - 1) * 16) + 8 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + 9 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + 10 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + 11 + texcolor_array_limit] = angle.s;

		att_PARASCAX[((handle - 1) * 16) + 12 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + 13 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + 14 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + 15 + texcolor_array_limit] = angle.s;
	}

	void rotateZCimage(int handle, coords3d angle)
	{
		quat q;

		// Basically we create 3 Quaternions, one for pitch, one for yaw, one for roll // and multiply those together. // the calculation below does the same, just shorter

		float p = angle.y * (M_PI / 180.0) / 2.0f; // Maybe expects "angle" to be a radian..
		float y = angle.z * (M_PI / 180.0) / 2.0f;
		float r = angle.x * (M_PI / 180.0) / 2.0f;

		float sinp = sin(p);
		float siny = sin(y);
		float sinr = sin(r);
		float cosp = cos(p);
		float cosy = cos(y);
		float cosr = cos(r);

		q.x = sinr * cosp * cosy - cosr * sinp * siny;
		q.y = cosr * sinp * cosy + sinr * cosp * siny;
		q.z = cosr * cosp * siny - sinr * sinp * cosy;
		q.s = cosr * cosp * cosy + sinr * sinp * siny;

		q = Qnormalize(q);

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 0 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 1 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 2 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 3 + texcolor_array_limit] = q.s;

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 4 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 5 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 6 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 7 + texcolor_array_limit] = q.s;

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 8 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 9 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 10 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 11 + texcolor_array_limit] = q.s;

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 12 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 13 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 14 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 15 + texcolor_array_limit] = q.s;
	}

	void QrotateZCimage(int handle, quat angle)
	{
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 0 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 1 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 2 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 3 + texcolor_array_limit] = angle.s; // We use MX.w, because it was spare.

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 4 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 5 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 6 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 7 + texcolor_array_limit] = angle.s;

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 8 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 9 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 10 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 11 + texcolor_array_limit] = angle.s;

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 12 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 13 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 14 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 15 + texcolor_array_limit] = angle.s;
	}

	void rotateHUDimage(int handle, coords3d angle)
	{
		quat q;

		// Basically we create 3 Quaternions, one for pitch, one for yaw, one for roll // and multiply those together. // the calculation below does the same, just shorter

		float p = angle.y * (M_PI / 180.0) / 2.0f; // Maybe expects "angle" to be a radian..
		float y = angle.z * (M_PI / 180.0) / 2.0f;
		float r = angle.x * (M_PI / 180.0) / 2.0f;

		float sinp = sin(p);
		float siny = sin(y);
		float sinr = sin(r);
		float cosp = cos(p);
		float cosy = cos(y);
		float cosr = cos(r);

		q.x = sinr * cosp * cosy - cosr * sinp * siny;
		q.y = cosr * sinp * cosy + sinr * cosp * siny;
		q.z = cosr * cosp * siny - sinr * sinp * cosy;
		q.s = cosr * cosp * cosy + sinr * sinp * siny;

		q = Qnormalize(q);


		// This is an example of a line that works
		//att_PARASCAX[((handle - 1) * 16) + (HUD_texcolor_offset)+0 + (texcolor_array_limit * 2)] = scale;

		// rotateimage
	//  att_PARASCAX[((handle - 1) * 16) + 0 + texcolor_array_limit] = q.x;

		// rotateZCimage
	//  att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 0 + texcolor_array_limit] = q.x;


		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 0 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 1 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 2 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 3 + texcolor_array_limit] = q.s;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 4 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 5 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 6 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 7 + texcolor_array_limit] = q.s;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 8 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 9 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 10 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 11 + texcolor_array_limit] = q.s;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 12 + texcolor_array_limit] = q.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 13 + texcolor_array_limit] = q.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 14 + texcolor_array_limit] = q.z;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 15 + texcolor_array_limit] = q.s;
	}

	void QrotateHUDimage(int handle, quat angle)
	{
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 0 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 1 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 2 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 3 + texcolor_array_limit] = angle.s; // We use MX.w, because it was spare.

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 4 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 5 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 6 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 7 + (texcolor_array_limit * 2)] = angle.s;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 8 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 9 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 10 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 11 + texcolor_array_limit] = angle.s;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 12 + texcolor_array_limit] = angle.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 13 + texcolor_array_limit] = angle.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 14 + texcolor_array_limit] = angle.z;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 15 + texcolor_array_limit] = angle.s;
	}

	void glassimage(int handle, float opac)
	{
		att_PARASCAX[((handle - 1) * 16) + 3] = opac;
		att_PARASCAX[((handle - 1) * 16) + 7] = opac;
		att_PARASCAX[((handle - 1) * 16) + 11] = opac;
		att_PARASCAX[((handle - 1) * 16) + 15] = opac;
	}

	void planeimage(int handle, coords3d normal)
	{
		att_PARASCAX[((handle - 1) * 16) + 1 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + 2 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + 3 + (texcolor_array_limit * 2)] = -normal.z;

		att_PARASCAX[((handle - 1) * 16) + 5 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + 6 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + 7 + (texcolor_array_limit * 2)] = -normal.z;

		att_PARASCAX[((handle - 1) * 16) + 9 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + 10 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + 11 + (texcolor_array_limit * 2)] = -normal.z;

		att_PARASCAX[((handle - 1) * 16) + 13 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + 14 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + 15 + (texcolor_array_limit * 2)] = -normal.z;
	}

	void glassZCimage(int handle, float opac)
	{
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 3] = opac;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 7] = opac;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 11] = opac;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 15] = opac;
	}

	void planeZCimage(int handle, coords3d normal)
	{
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 1 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 2 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 3 + (texcolor_array_limit * 2)] = -normal.z;

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 5 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 6 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 7 + (texcolor_array_limit * 2)] = -normal.z;

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 9 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 10 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 11 + (texcolor_array_limit * 2)] = -normal.z;

		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 13 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 14 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 15 + (texcolor_array_limit * 2)] = -normal.z;
	}

	void glassHUDimage(int handle, float opac)
	{
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 3] = opac;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 7] = opac;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 11] = opac;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 15] = opac;
	}

	void planeHUDimage(int handle, coords3d normal)
	{
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 1 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 2 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 3 + (texcolor_array_limit * 2)] = -normal.z;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 5 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 6 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 7 + (texcolor_array_limit * 2)] = -normal.z;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 9 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 10 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 11 + (texcolor_array_limit * 2)] = -normal.z;

		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 13 + (texcolor_array_limit * 2)] = normal.x;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 14 + (texcolor_array_limit * 2)] = normal.y;
		att_PARASCAX[((handle - 1) * 16) + HUD_texcolor_offset + 15 + (texcolor_array_limit * 2)] = -normal.z;
	}

	coords3d normalize(coords3d normal)
	{
		float d = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

		if (d == 0.0)
		{
			//cout<<"Normalize error: zero length vector!\n";

			normal.x = 0;
			normal.y = 0;
			normal.z = 0;
		}
		else
		{
			normal.x /= d;
			normal.y /= d;
			normal.z /= d;
		}

		return normal;
	}

	quat Qnormalize(quat q)
	{
		float d = sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.s * q.s);

		if (d == 0.0)
		{
			//cout<<"Normalize error: zero length vector!\n";

			q.x = 0;
			q.y = 0;
			q.z = 0;
			q.s = 0;
		}
		else
		{
			q.x /= d;
			q.y /= d;
			q.z /= d;
			q.s /= d;
		}

		return q;
	}

	coords3d normcrossprod(coords3d vec1, coords3d vec2)
	{
		coords3d out;

		out.x = vec1.y * vec2.z - vec1.z * vec2.y;
		out.y = vec1.z * vec2.x - vec1.x * vec2.z;
		out.z = vec1.x * vec2.y - vec1.y * vec2.x;

		out = normalize(out);

		return out;
	}

	float dotproduct(coords3d vec1, coords3d vec2)
	{
		float v1[] = { vec1.x, vec1.y, vec1.z };
		float v2[] = { vec2.x, vec2.y, vec2.z };

		return inner_product(v1, v1 + 3, v2, 0.0);
	}

	coords3d rotatevector(coords3d norm, coords3d pos)
	{
		coords3d old_x = { 1, 0, 0 };

		coords3d new_x = { 0, 0, 0 };
		coords3d new_y = { 0, 0, 0 };
		coords3d new_z = { norm.x, norm.y, norm.z }; // Z has been minused.

		new_y = normcrossprod(new_z, old_x);
		new_x = normcrossprod(new_y, new_z);

		//cout << "New x axis = " << new_x.x << ", " << new_x.y << ", " << new_x.z << "\n";
		//cout << "New y axis = " << new_y.x << ", " << new_y.y << ", " << new_y.z << "\n";
		//cout << "New z axis = " << new_z.x << ", " << new_z.y << ", " << new_z.z << "\n";

		coords3d val;

		val.x = dotproduct(new_x, pos);
		val.y = dotproduct(new_y, pos);
		val.z = dotproduct(new_z, pos);

		//cout << "val = " << val.x << ", " << val.y << ", " << val.z << "\n";

		return val;
	}

	coords3d calc_triangle_normal(tri t)
	{
		coords3d vec1;
		coords3d vec2;
		coords3d tri_normal;

		vec1.x = t.c.x - t.a.x;
		vec1.y = t.c.y - t.a.y;
		vec1.z = t.c.z - t.a.z;

		vec2.x = t.b.x - t.a.x;
		vec2.y = t.b.y - t.a.y;
		vec2.z = t.b.z - t.a.z;

		tri_normal = normcrossprod(vec1, vec2);

		return tri_normal;
	}

	point vector_to_angle(coords3d vec)
	{
		float x = vec.x;
		float y = vec.y;
		float z = vec.z;

		float zrot = (atan2(x, y) * 57.2957) + 180.0;
		float xrot = atan2(z, sqrt((x * x) + (y * y))) * 57.2957;

		point rot = { zrot, xrot };

		return rot;
	}

	inline float ease_quad(float t, float start, float end, float fps)	// 'end' is the length so, it's start to start+end
	{
		t /= fps / 2;
		if (t < 1)
			return end / 2 * t * t + start;
		t--;
		return -end / 2 * (t * (t - 2) - 1) + start;
	}

	inline float ease_bounce(float t)
	{
		return fabs(sin((3.14 * (t + 1.0) * (t + 1.0)) * (1.0 - t)));
	}

	inline float ease_in(float t)
	{
		return (t * t);
	}

	inline float ease_out(float t)
	{
		return -(t * (t - 2));
	}

	inline float ease_inout(float t)
	{
		return t < 0.5 ? 2 * t * t : 1 - pow(-2 * t + 2, 2) / 2;
	}

	inline float ease_out_elastic(float t)
	{
		float p = 0.3;
		return pow(2, -10 * t) * sin((t - p / 4) * (2 * 3.14159) / p) + 1;
	}

	inline float ease_halfcircle(float t) // Custom 0 to 1 to 0 ( 0.00029 to 1.0 to 0.00029 )
	{
		return 1.0 * cos(((t * 180.0) - 90) * 0.01745);
	}

	inline point ease_lemniscate(float t)
	{
		// Bernoulli

		float scale = 2.0 / (3 - cos(2 * t));

		point p;

		p.x = scale * cos(t);
		p.y = scale * sin(2.0 * t) / 2.0;

		// Gerono

		//point p = { cos(t), sin(2.0f * t) / 2.0f };

		return p;
	}

	inline float modulo(float a, float b)
	{
		return fmod((fmod(a, b) + b), b);
	}

	inline float AngleOfLine(point p0, point p1)
	{
		double angle = atan2(p1.x - p0.x, p0.y - p1.y) * 57.29578;
		angle = modulo(angle, 360);

		return angle;
	}

	inline point cosine(float length, float angle)
	{
		point r;

		r.x = length * cos((angle - 90.0f) * 0.01745);
		r.y = length * sin((angle - 90.0f) * 0.01745);

		return r;
	}

	inline ipoint cosine_int(float length, float angle)
	{
		ipoint r;

		r.x = round(length * cos((angle - 90.0f) * 0.01745));
		r.y = round(length * sin((angle - 90.0f) * 0.01745));

		return r;
	}

	inline float DistanceBetween(point a, point b)
	{
		return (sqrt(((b.x - a.x) * (b.x - a.x)) + ((b.y - a.y) * (b.y - a.y))));
	}

	inline float DistanceBetween3D(coords3d a, coords3d b)
	{
		return (sqrt(((b.x - a.x) * (b.x - a.x)) + ((b.y - a.y) * (b.y - a.y)) + ((b.z - a.z) * (b.z - a.z))));
	}

	inline float AngleDelta(point origin, float angle, point target)
	{
		float tanangle = (atan2(target.x - origin.x, origin.y - target.y) * 57.2957);
		float origin_angle = modulo(angle, 360);
		float sum = 0;

		if (origin_angle > tanangle)
		{
			sum = -modulo((origin_angle - tanangle), 360);

			if (sum < -179)
				sum = (360 - -(sum));
		}
		else
		{
			sum = modulo((tanangle - origin_angle), 360);

			if (sum < 0.00)
				sum = 0;
		}

		return sum;
	}

	inline coords3d quaternionToEuler(quat q)
	{
		coords3d euler;

		// roll (x-axis rotation)
		float sinr_cosp = 2.0f * float(q.s * q.x + q.y * q.z);
		float cosr_cosp = 1.0f - 2.0f * float(q.x * q.x + q.y * q.y);
		euler.x = std::atan2(sinr_cosp, cosr_cosp) * 57.2957;

		// pitch (y-axis rotation)
		float sinp = 2.0f * float(q.s * q.y - q.z * q.x);
		if (std::abs(sinp) >= 1)
			euler.y = std::copysign(M_PI / 2, sinp) * 57.2957; // use 90 degrees if out of range
		else
			euler.y = std::asin(sinp) * 57.2957;

		// yaw (z-axis rotation)
		float siny_cosp = 2.0f * float(q.s * q.z + q.x * q.y);
		float cosy_cosp = 1.0f - 2.0f * float(q.y * q.y + q.z * q.z);
		euler.z = std::atan2(siny_cosp, cosy_cosp) * 57.2957;

		return euler;
	}

	void backfacecull(bool state)
	{
		if (state == true)
			glEnable(GL_CULL_FACE);
		else if (state == false)
			glDisable(GL_CULL_FACE);
	}

	void wireframe(bool state)
	{
		wireframe_mode = state;
	}

	coords3d facepointtopoint(coords3d a, coords3d b) // object, origin
	{
		a.x = a.x - b.x; // Normalise to origin
		a.y = a.y - b.y;
		a.z = a.z - b.z;

		b.x = 0.0;
		b.y = 0.0;
		b.z = 0.0;

		point objectpos;
		objectpos.x = a.x;
		objectpos.y = a.y;

		point originpos;
		originpos.x = b.x;
		originpos.y = b.y;

		float z_angle = (AngleOfLine(objectpos, originpos) - 180.0);

		float dist = DistanceBetween(objectpos, originpos);
		objectpos.x = -dist;
		objectpos.y = a.z;

		float x_angle = AngleOfLine(objectpos, originpos);

		coords3d result;
		result.x = x_angle;
		result.y = 0.0;
		result.z = z_angle;

		return result;
	}

	void faceimagetopoint(int handle, float x, float y, float z)
	{
		coords3d lookvec;

		coords3d objectpos;
		objectpos.x = att_PARASCAX[((handle - 1) * 16) + 0] * 100.0;
		objectpos.y = att_PARASCAX[((handle - 1) * 16) + 1] * 100.0;
		objectpos.z = att_PARASCAX[((handle - 1) * 16) + 2] * 100.0;

		coords3d targetpos;
		targetpos.x = x;
		targetpos.y = y;
		targetpos.z = z;

		lookvec = facepointtopoint(objectpos, targetpos);

		rotateimage(handle, lookvec);
	}

	void faceimagetocamera(int handle, coords3d camera)
	{
		coords3d objectpos;
		coords3d camerapos;
		coords3d lookvec;

		objectpos.x = att_PARASCAX[((handle - 1) * 16) + 0] * 100.0;
		objectpos.y = att_PARASCAX[((handle - 1) * 16) + 1] * 100.0;
		objectpos.z = att_PARASCAX[((handle - 1) * 16) + 2] * 100.0;

		camerapos.x = camera.x * 100.0;
		camerapos.y = camera.y * 100.0;
		camerapos.z = camera.z * 100.0;

		lookvec = facepointtopoint(objectpos, camerapos);

		rotateimage(handle, lookvec);
	}

	void faceZCimagetocamera(int handle, coords3d cameraZC)
	{
		coords3d objectpos;
		coords3d camerapos;
		coords3d lookvec;

		objectpos.x = att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 0] * 100.0;
		objectpos.y = att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 1] * 100.0;
		objectpos.z = att_PARASCAX[((handle - 1) * 16) + ZC_texcolor_offset + 2] * 100.0;

		camerapos.x = cameraZC.x * 100.0;
		camerapos.y = cameraZC.y * 100.0;
		camerapos.z = cameraZC.z * 100.0;

		lookvec = facepointtopoint(objectpos, camerapos);

		rotateZCimage(handle, lookvec);
	}

	void faceobjecttocamera(int handle, coords3d camera)
	{
		for (int i = 0; i < object_i[handle - 1].count; i++)
			faceimagetocamera(object_i[handle - 1].startimage + i, camera);
	}

	void faceZCobjecttocamera(int handle, coords3d camera)
	{
		for (int i = 0; i < ZCobject_i[handle - 1].count; i++)
			faceZCimagetocamera(ZCobject_i[handle - 1].startimage + i, camera);
	}

	//static void error_callback(int error, const char* description)
	//{
	//	fputs(description, stderr);
	//}

	void loadPNG(vector<unsigned char>& buffer, string filename)
	{
		std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

		//get filesize
		std::streamsize size = 0;
		if (file.seekg(0, std::ios::end).good()) size = file.tellg();
		if (file.seekg(0, std::ios::beg).good()) size -= file.tellg();

		//read contents of the file into the vector
		if (size > 0)
		{
			buffer.resize((size_t)size);
			file.read((char*)(&buffer[0]), size);
		}
		else buffer.clear();
	}

	int decodePNG(vector<unsigned char>& out_image, unsigned long& image_width, unsigned long& image_height, const unsigned char* in_png, size_t in_size, bool convert_to_rgba32)
	{
		// picoPNG version 20101224
		// Copyright (c) 2005-2010 Lode Vandevenne
		//
		// This software is provided 'as-is', without any express or implied
		// warranty. In no event will the authors be held liable for any damages
		// arising from the use of this software.
		//
		// Permission is granted to anyone to use this software for any purpose,
		// including commercial applications, and to alter it and redistribute it
		// freely, subject to the following restrictions:
		//
		//     1. The origin of this software must not be misrepresented; you must not
		//     claim that you wrote the original software. If you use this software
		//     in a product, an acknowledgment in the product documentation would be
		//     appreciated but is not required.
		//     2. Altered source versions must be plainly marked as such, and must not be
		//     misrepresented as being the original software.
		//     3. This notice may not be removed or altered from any source distribution.

		// picoPNG is a PNG decoder in one C++ function of around 500 lines. Use picoPNG for
		// programs that need only 1 .cpp file. Since it's a single function, it's very limited,
		// it can convert a PNG to raw pixel data either converted to 32-bit RGBA color or
		// with no color conversion at all. For anything more complex, another tiny library
		// is available: LodePNG (lodepng.c(pp)), which is a single source and header file.
		// Apologies for the compact code style, it's to make this tiny.

		static const unsigned long LENBASE[29] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
		static const unsigned long LENEXTRA[29] = { 0,0,0,0,0,0,0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5,  0 };
		static const unsigned long DISTBASE[30] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };
		static const unsigned long DISTEXTRA[30] = { 0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13 };
		static const unsigned long CLCL[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 }; //code length code lengths
		struct Zlib //nested functions for zlib decompression
		{
			static unsigned long readBitFromStream(size_t& bitp, const unsigned char* bits) { unsigned long result = (bits[bitp >> 3] >> (bitp & 0x7)) & 1; bitp++; return result; }
			static unsigned long readBitsFromStream(size_t& bitp, const unsigned char* bits, size_t nbits)
			{
				unsigned long result = 0;
				for (size_t i = 0; i < nbits; i++) result += (readBitFromStream(bitp, bits)) << i;
				return result;
			}
			struct HuffmanTree
			{
				int makeFromLengths(const std::vector<unsigned long>& bitlen, unsigned long maxbitlen)
				{ //make tree given the lengths
					unsigned long numcodes = (unsigned long)(bitlen.size()), treepos = 0, nodefilled = 0;
					std::vector<unsigned long> tree1d(numcodes), blcount(maxbitlen + 1, 0), nextcode(maxbitlen + 1, 0);
					for (unsigned long bits = 0; bits < numcodes; bits++) blcount[bitlen[bits]]++; //count number of instances of each code length
					for (unsigned long bits = 1; bits <= maxbitlen; bits++) nextcode[bits] = (nextcode[bits - 1] + blcount[bits - 1]) << 1;
					for (unsigned long n = 0; n < numcodes; n++) if (bitlen[n] != 0) tree1d[n] = nextcode[bitlen[n]]++; //generate all the codes
					tree2d.clear(); tree2d.resize(numcodes * 2, 32767); //32767 here means the tree2d isn't filled there yet
					for (unsigned long n = 0; n < numcodes; n++) //the codes
						for (unsigned long i = 0; i < bitlen[n]; i++) //the bits for this code
						{
							unsigned long bit = (tree1d[n] >> (bitlen[n] - i - 1)) & 1;
							if (treepos > numcodes - 2) return 55;
							if (tree2d[2 * treepos + bit] == 32767) //not yet filled in
							{
								if (i + 1 == bitlen[n]) { tree2d[2 * treepos + bit] = n; treepos = 0; } //last bit
								else { tree2d[2 * treepos + bit] = ++nodefilled + numcodes; treepos = nodefilled; } //addresses are encoded as values > numcodes
							}
							else treepos = tree2d[2 * treepos + bit] - numcodes; //subtract numcodes from address to get address value
						}
					return 0;
				}
				int decode(bool& decoded, unsigned long& result, size_t& treepos, unsigned long bit) const
				{ //Decodes a symbol from the tree
					unsigned long numcodes = (unsigned long)tree2d.size() / 2;
					if (treepos >= numcodes) return 11; //error: you appeared outside the codetree
					result = tree2d[2 * treepos + bit];
					decoded = (result < numcodes);
					treepos = decoded ? 0 : result - numcodes;
					return 0;
				}
				std::vector<unsigned long> tree2d; //2D representation of a huffman tree: The one dimension is "0" or "1", the other contains all nodes and leaves of the tree.
			};
			struct Inflator
			{
				int error;
				void inflate(std::vector<unsigned char>& out, const std::vector<unsigned char>& in, size_t inpos = 0)
				{
					size_t bp = 0, pos = 0; //bit pointer and byte pointer
					error = 0;
					unsigned long BFINAL = 0;
					while (!BFINAL && !error)
					{
						if (bp >> 3 >= in.size()) { error = 52; return; } //error, bit pointer will jump past memory
						BFINAL = readBitFromStream(bp, &in[inpos]);
						unsigned long BTYPE = readBitFromStream(bp, &in[inpos]); BTYPE += 2 * readBitFromStream(bp, &in[inpos]);
						if (BTYPE == 3) { error = 20; return; } //error: invalid BTYPE
						else if (BTYPE == 0) inflateNoCompression(out, &in[inpos], bp, pos, in.size());
						else inflateHuffmanBlock(out, &in[inpos], bp, pos, in.size(), BTYPE);
					}
					if (!error) out.resize(pos); //Only now we know the true size of out, resize it to that
				}
				void generateFixedTrees(HuffmanTree& tree, HuffmanTree& treeD) //get the tree of a deflated block with fixed tree
				{
					std::vector<unsigned long> bitlen(288, 8), bitlenD(32, 5);;
					for (size_t i = 144; i <= 255; i++) bitlen[i] = 9;
					for (size_t i = 256; i <= 279; i++) bitlen[i] = 7;
					tree.makeFromLengths(bitlen, 15);
					treeD.makeFromLengths(bitlenD, 15);
				}
				HuffmanTree codetree, codetreeD, codelengthcodetree; //the code tree for Huffman codes, dist codes, and code length codes
				unsigned long huffmanDecodeSymbol(const unsigned char* in, size_t& bp, const HuffmanTree& codetree, size_t inlength)
				{ //decode a single symbol from given list of bits with given code tree. return value is the symbol
					bool decoded; unsigned long ct;
					for (size_t treepos = 0;;)
					{
						if ((bp & 0x07) == 0 && (bp >> 3) > inlength) { error = 10; return 0; } //error: end reached without endcode
						error = codetree.decode(decoded, ct, treepos, readBitFromStream(bp, in)); if (error) return 0; //stop, an error happened
						if (decoded) return ct;
					}
				}
				void getTreeInflateDynamic(HuffmanTree& tree, HuffmanTree& treeD, const unsigned char* in, size_t& bp, size_t inlength)
				{ //get the tree of a deflated block with dynamic tree, the tree itself is also Huffman compressed with a known tree
					std::vector<unsigned long> bitlen(288, 0), bitlenD(32, 0);
					if (bp >> 3 >= inlength - 2) { error = 49; return; } //the bit pointer is or will go past the memory
					size_t HLIT = readBitsFromStream(bp, in, 5) + 257; //number of literal/length codes + 257
					size_t HDIST = readBitsFromStream(bp, in, 5) + 1; //number of dist codes + 1
					size_t HCLEN = readBitsFromStream(bp, in, 4) + 4; //number of code length codes + 4
					std::vector<unsigned long> codelengthcode(19); //lengths of tree to decode the lengths of the dynamic tree
					for (size_t i = 0; i < 19; i++) codelengthcode[CLCL[i]] = (i < HCLEN) ? readBitsFromStream(bp, in, 3) : 0;
					error = codelengthcodetree.makeFromLengths(codelengthcode, 7); if (error) return;
					size_t i = 0, replength;
					while (i < HLIT + HDIST)
					{
						unsigned long code = huffmanDecodeSymbol(in, bp, codelengthcodetree, inlength); if (error) return;
						if (code <= 15) { if (i < HLIT) bitlen[i++] = code; else bitlenD[i++ - HLIT] = code; } //a length code
						else if (code == 16) //repeat previous
						{
							if (bp >> 3 >= inlength) { error = 50; return; } //error, bit pointer jumps past memory
							replength = 3 + readBitsFromStream(bp, in, 2);
							unsigned long value; //set value to the previous code
							if ((i - 1) < HLIT) value = bitlen[i - 1];
							else value = bitlenD[i - HLIT - 1];
							for (size_t n = 0; n < replength; n++) //repeat this value in the next lengths
							{
								if (i >= HLIT + HDIST) { error = 13; return; } //error: i is larger than the amount of codes
								if (i < HLIT) bitlen[i++] = value; else bitlenD[i++ - HLIT] = value;
							}
						}
						else if (code == 17) //repeat "0" 3-10 times
						{
							if (bp >> 3 >= inlength) { error = 50; return; } //error, bit pointer jumps past memory
							replength = 3 + readBitsFromStream(bp, in, 3);
							for (size_t n = 0; n < replength; n++) //repeat this value in the next lengths
							{
								if (i >= HLIT + HDIST) { error = 14; return; } //error: i is larger than the amount of codes
								if (i < HLIT) bitlen[i++] = 0; else bitlenD[i++ - HLIT] = 0;
							}
						}
						else if (code == 18) //repeat "0" 11-138 times
						{
							if (bp >> 3 >= inlength) { error = 50; return; } //error, bit pointer jumps past memory
							replength = 11 + readBitsFromStream(bp, in, 7);
							for (size_t n = 0; n < replength; n++) //repeat this value in the next lengths
							{
								if (i >= HLIT + HDIST) { error = 15; return; } //error: i is larger than the amount of codes
								if (i < HLIT) bitlen[i++] = 0; else bitlenD[i++ - HLIT] = 0;
							}
						}
						else { error = 16; return; } //error: somehow an unexisting code appeared. This can never happen.
					}
					if (bitlen[256] == 0) { error = 64; return; } //the length of the end code 256 must be larger than 0
					error = tree.makeFromLengths(bitlen, 15); if (error) return; //now we've finally got HLIT and HDIST, so generate the code trees, and the function is done
					error = treeD.makeFromLengths(bitlenD, 15); if (error) return;
				}
				void inflateHuffmanBlock(std::vector<unsigned char>& out, const unsigned char* in, size_t& bp, size_t& pos, size_t inlength, unsigned long btype)
				{
					if (btype == 1) { generateFixedTrees(codetree, codetreeD); }
					else if (btype == 2) { getTreeInflateDynamic(codetree, codetreeD, in, bp, inlength); if (error) return; }
					for (;;)
					{
						unsigned long code = huffmanDecodeSymbol(in, bp, codetree, inlength); if (error) return;
						if (code == 256) return; //end code
						else if (code <= 255) //literal symbol
						{
							if (pos >= out.size()) out.resize((pos + 1) * 2); //reserve more room
							out[pos++] = (unsigned char)(code);
						}
						else if (code >= 257 && code <= 285) //length code
						{
							size_t length = LENBASE[code - 257], numextrabits = LENEXTRA[code - 257];
							if ((bp >> 3) >= inlength) { error = 51; return; } //error, bit pointer will jump past memory
							length += readBitsFromStream(bp, in, numextrabits);
							unsigned long codeD = huffmanDecodeSymbol(in, bp, codetreeD, inlength); if (error) return;
							if (codeD > 29) { error = 18; return; } //error: invalid dist code (30-31 are never used)
							unsigned long dist = DISTBASE[codeD], numextrabitsD = DISTEXTRA[codeD];
							if ((bp >> 3) >= inlength) { error = 51; return; } //error, bit pointer will jump past memory
							dist += readBitsFromStream(bp, in, numextrabitsD);
							size_t start = pos, back = start - dist; //backwards
							if (pos + length >= out.size()) out.resize((pos + length) * 2); //reserve more room
							for (size_t i = 0; i < length; i++) { out[pos++] = out[back++]; if (back >= start) back = start - dist; }
						}
					}
				}
				void inflateNoCompression(std::vector<unsigned char>& out, const unsigned char* in, size_t& bp, size_t& pos, size_t inlength)
				{
					while ((bp & 0x7) != 0) bp++; //go to first boundary of byte
					size_t p = bp / 8;
					if (p >= inlength - 4) { error = 52; return; } //error, bit pointer will jump past memory
					unsigned long LEN = in[p] + 256 * in[p + 1], NLEN = in[p + 2] + 256 * in[p + 3]; p += 4;
					if (LEN + NLEN != 65535) { error = 21; return; } //error: NLEN is not one's complement of LEN
					if (pos + LEN >= out.size()) out.resize(pos + LEN);
					if (p + LEN > inlength) { error = 23; return; } //error: reading outside of in buffer
					for (unsigned long n = 0; n < LEN; n++) out[pos++] = in[p++]; //read LEN bytes of literal data
					bp = p * 8;
				}
			};
			int decompress(std::vector<unsigned char>& out, const std::vector<unsigned char>& in) //returns error value
			{
				Inflator inflator;
				if (in.size() < 2) { return 53; } //error, size of zlib data too small
				if ((in[0] * 256 + in[1]) % 31 != 0) { return 24; } //error: 256 * in[0] + in[1] must be a multiple of 31, the FCHECK value is supposed to be made that way
				unsigned long CM = in[0] & 15, CINFO = (in[0] >> 4) & 15, FDICT = (in[1] >> 5) & 1;
				if (CM != 8 || CINFO > 7) { return 25; } //error: only compression method 8: inflate with sliding window of 32k is supported by the PNG spec
				if (FDICT != 0) { return 26; } //error: the specification of PNG says about the zlib stream: "The additional flags shall not specify a preset dictionary."
				inflator.inflate(out, in, 2);
				return inflator.error; //note: adler32 checksum was skipped and ignored
			}
		};
		struct PNG //nested functions for PNG decoding
		{
			struct Info
			{
				unsigned long width, height, colorType, bitDepth, compressionMethod, filterMethod, interlaceMethod, key_r, key_g, key_b;
				bool key_defined; //is a transparent color key given?
				std::vector<unsigned char> palette;
			} info;
			int error;
			void decode(std::vector<unsigned char>& out, const unsigned char* in, size_t size, bool convert_to_rgba32)
			{
				error = 0;
				if (size == 0 || in == 0) { error = 48; return; } //the given data is empty
				readPngHeader(&in[0], size); if (error) return;
				size_t pos = 33; //first byte of the first chunk after the header
				std::vector<unsigned char> idat; //the data from idat chunks
				bool IEND = false, known_type = true;
				info.key_defined = false;
				while (!IEND) //loop through the chunks, ignoring unknown chunks and stopping at IEND chunk. IDAT data is put at the start of the in buffer
				{
					if (pos + 8 >= size) { error = 30; return; } //error: size of the in buffer too small to contain next chunk
					size_t chunkLength = read32bitInt(&in[pos]); pos += 4;
					if (chunkLength > 2147483647) { error = 63; return; }
					if (pos + chunkLength >= size) { error = 35; return; } //error: size of the in buffer too small to contain next chunk
					if (in[pos + 0] == 'I' && in[pos + 1] == 'D' && in[pos + 2] == 'A' && in[pos + 3] == 'T') //IDAT chunk, containing compressed image data
					{
						idat.insert(idat.end(), &in[pos + 4], &in[pos + 4 + chunkLength]);
						pos += (4 + chunkLength);
					}
					else if (in[pos + 0] == 'I' && in[pos + 1] == 'E' && in[pos + 2] == 'N' && in[pos + 3] == 'D') { pos += 4; IEND = true; }
					else if (in[pos + 0] == 'P' && in[pos + 1] == 'L' && in[pos + 2] == 'T' && in[pos + 3] == 'E') //palette chunk (PLTE)
					{
						pos += 4; //go after the 4 letters
						info.palette.resize(4 * (chunkLength / 3));
						if (info.palette.size() > (4 * 256)) { error = 38; return; } //error: palette too big
						for (size_t i = 0; i < info.palette.size(); i += 4)
						{
							for (size_t j = 0; j < 3; j++) info.palette[i + j] = in[pos++]; //RGB
							info.palette[i + 3] = 255; //alpha
						}
					}
					else if (in[pos + 0] == 't' && in[pos + 1] == 'R' && in[pos + 2] == 'N' && in[pos + 3] == 'S') //palette transparency chunk (tRNS)
					{
						pos += 4; //go after the 4 letters
						if (info.colorType == 3)
						{
							if (4 * chunkLength > info.palette.size()) { error = 39; return; } //error: more alpha values given than there are palette entries
							for (size_t i = 0; i < chunkLength; i++) info.palette[4 * i + 3] = in[pos++];
						}
						else if (info.colorType == 0)
						{
							if (chunkLength != 2) { error = 40; return; } //error: this chunk must be 2 bytes for greyscale image
							info.key_defined = 1; info.key_r = info.key_g = info.key_b = 256 * in[pos] + in[pos + 1]; pos += 2;
						}
						else if (info.colorType == 2)
						{
							if (chunkLength != 6) { error = 41; return; } //error: this chunk must be 6 bytes for RGB image
							info.key_defined = 1;
							info.key_r = 256 * in[pos] + in[pos + 1]; pos += 2;
							info.key_g = 256 * in[pos] + in[pos + 1]; pos += 2;
							info.key_b = 256 * in[pos] + in[pos + 1]; pos += 2;
						}
						else { error = 42; return; } //error: tRNS chunk not allowed for other color models
					}
					else //it's not an implemented chunk type, so ignore it: skip over the data
					{
						if (!(in[pos + 0] & 32)) { error = 69; return; } //error: unknown critical chunk (5th bit of first byte of chunk type is 0)
						pos += (chunkLength + 4); //skip 4 letters and uninterpreted data of unimplemented chunk
						known_type = false;
					}
					pos += 4; //step over CRC (which is ignored)
				}
				unsigned long bpp = getBpp(info);
				std::vector<unsigned char> scanlines(((info.width * (info.height * bpp + 7)) / 8) + info.height); //now the out buffer will be filled
				Zlib zlib; //decompress with the Zlib decompressor
				error = zlib.decompress(scanlines, idat); if (error) return; //stop if the zlib decompressor returned an error
				size_t bytewidth = (bpp + 7) / 8, outlength = (info.height * info.width * bpp + 7) / 8;
				out.resize(outlength); //time to fill the out buffer
				unsigned char* out_ = outlength ? &out[0] : 0; //use a regular pointer to the std::vector for faster code if compiled without optimization
				if (info.interlaceMethod == 0) //no interlace, just filter
				{
					size_t linestart = 0, linelength = (info.width * bpp + 7) / 8; //length in bytes of a scanline, excluding the filtertype byte
					if (bpp >= 8) //byte per byte
						for (unsigned long y = 0; y < info.height; y++)
						{
							unsigned long filterType = scanlines[linestart];
							const unsigned char* prevline = (y == 0) ? 0 : &out_[(y - 1) * info.width * bytewidth];
							unFilterScanline(&out_[linestart - y], &scanlines[linestart + 1], prevline, bytewidth, filterType, linelength); if (error) return;
							linestart += (1 + linelength); //go to start of next scanline
						}
					else //less than 8 bits per pixel, so fill it up bit per bit
					{
						std::vector<unsigned char> templine((info.width * bpp + 7) >> 3); //only used if bpp < 8
						for (size_t y = 0, obp = 0; y < info.height; y++)
						{
							unsigned long filterType = scanlines[linestart];
							const unsigned char* prevline = (y == 0) ? 0 : &out_[(y - 1) * info.width * bytewidth];
							unFilterScanline(&templine[0], &scanlines[linestart + 1], prevline, bytewidth, filterType, linelength); if (error) return;
							for (size_t bp = 0; bp < info.width * bpp;) setBitOfReversedStream(obp, out_, readBitFromReversedStream(bp, &templine[0]));
							linestart += (1 + linelength); //go to start of next scanline
						}
					}
				}
				else //interlaceMethod is 1 (Adam7)
				{
					size_t passw[7] = { (info.width + 7) / 8, (info.width + 3) / 8, (info.width + 3) / 4, (info.width + 1) / 4, (info.width + 1) / 2, (info.width + 0) / 2, (info.width + 0) / 1 };
					size_t passh[7] = { (info.height + 7) / 8, (info.height + 7) / 8, (info.height + 3) / 8, (info.height + 3) / 4, (info.height + 1) / 4, (info.height + 1) / 2, (info.height + 0) / 2 };
					size_t passstart[7] = { 0 };
					size_t pattern[28] = { 0,4,0,2,0,1,0,0,0,4,0,2,0,1,8,8,4,4,2,2,1,8,8,8,4,4,2,2 }; //values for the adam7 passes
					for (int i = 0; i < 6; i++) passstart[i + 1] = passstart[i] + passh[i] * ((passw[i] ? 1 : 0) + (passw[i] * bpp + 7) / 8);
					std::vector<unsigned char> scanlineo((info.width * bpp + 7) / 8), scanlinen((info.width * bpp + 7) / 8); //"old" and "new" scanline
					for (int i = 0; i < 7; i++)
						adam7Pass(&out_[0], &scanlinen[0], &scanlineo[0], &scanlines[passstart[i]], info.width, pattern[i], pattern[i + 7], pattern[i + 14], pattern[i + 21], passw[i], passh[i], bpp);
				}
				if (convert_to_rgba32 && (info.colorType != 6 || info.bitDepth != 8)) //conversion needed
				{
					std::vector<unsigned char> data = out;
					error = convert(out, &data[0], info, info.width, info.height);
				}
			}
			void readPngHeader(const unsigned char* in, size_t inlength) //read the information from the header and store it in the Info
			{
				if (inlength < 29) { error = 27; return; } //error: the data length is smaller than the length of the header
				if (in[0] != 137 || in[1] != 80 || in[2] != 78 || in[3] != 71 || in[4] != 13 || in[5] != 10 || in[6] != 26 || in[7] != 10) { error = 28; return; } //no PNG signature
				if (in[12] != 'I' || in[13] != 'H' || in[14] != 'D' || in[15] != 'R') { error = 29; return; } //error: it doesn't start with a IHDR chunk!
				info.width = read32bitInt(&in[16]); info.height = read32bitInt(&in[20]);
				info.bitDepth = in[24]; info.colorType = in[25];
				info.compressionMethod = in[26]; if (in[26] != 0) { error = 32; return; } //error: only compression method 0 is allowed in the specification
				info.filterMethod = in[27]; if (in[27] != 0) { error = 33; return; } //error: only filter method 0 is allowed in the specification
				info.interlaceMethod = in[28]; if (in[28] > 1) { error = 34; return; } //error: only interlace methods 0 and 1 exist in the specification
				error = checkColorValidity(info.colorType, info.bitDepth);
			}
			void unFilterScanline(unsigned char* recon, const unsigned char* scanline, const unsigned char* precon, size_t bytewidth, unsigned long filterType, size_t length)
			{
				switch (filterType)
				{
				case 0: for (size_t i = 0; i < length; i++) recon[i] = scanline[i]; break;
				case 1:
					for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i];
					for (size_t i = bytewidth; i < length; i++) recon[i] = scanline[i] + recon[i - bytewidth];
					break;
				case 2:
					if (precon) for (size_t i = 0; i < length; i++) recon[i] = scanline[i] + precon[i];
					else       for (size_t i = 0; i < length; i++) recon[i] = scanline[i];
					break;
				case 3:
					if (precon)
					{
						for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i] + precon[i] / 2;
						for (size_t i = bytewidth; i < length; i++) recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
					}
					else
					{
						for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i];
						for (size_t i = bytewidth; i < length; i++) recon[i] = scanline[i] + recon[i - bytewidth] / 2;
					}
					break;
				case 4:
					if (precon)
					{
						for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i] + paethPredictor(0, precon[i], 0);
						for (size_t i = bytewidth; i < length; i++) recon[i] = scanline[i] + paethPredictor(recon[i - bytewidth], precon[i], precon[i - bytewidth]);
					}
					else
					{
						for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i];
						for (size_t i = bytewidth; i < length; i++) recon[i] = scanline[i] + paethPredictor(recon[i - bytewidth], 0, 0);
					}
					break;
				default: error = 36; return; //error: unexisting filter type given
				}
			}
			void adam7Pass(unsigned char* out, unsigned char* linen, unsigned char* lineo, const unsigned char* in, unsigned long w, size_t passleft, size_t passtop, size_t spacex, size_t spacey, size_t passw, size_t passh, unsigned long bpp)
			{ //filter and reposition the pixels into the output when the image is Adam7 interlaced. This function can only do it after the full image is already decoded. The out buffer must have the correct allocated memory size already.
				if (passw == 0) return;
				size_t bytewidth = (bpp + 7) / 8, linelength = 1 + ((bpp * passw + 7) / 8);
				for (unsigned long y = 0; y < passh; y++)
				{
					unsigned char filterType = in[y * linelength], * prevline = (y == 0) ? 0 : lineo;
					unFilterScanline(linen, &in[y * linelength + 1], prevline, bytewidth, filterType, (w * bpp + 7) / 8); if (error) return;
					if (bpp >= 8) for (size_t i = 0; i < passw; i++) for (size_t b = 0; b < bytewidth; b++) //b = current byte of this pixel
						out[bytewidth * w * (passtop + spacey * y) + bytewidth * (passleft + spacex * i) + b] = linen[bytewidth * i + b];
					else for (size_t i = 0; i < passw; i++)
					{
						size_t obp = bpp * w * (passtop + spacey * y) + bpp * (passleft + spacex * i), bp = i * bpp;
						for (size_t b = 0; b < bpp; b++) setBitOfReversedStream(obp, out, readBitFromReversedStream(bp, &linen[0]));
					}
					unsigned char* temp = linen; linen = lineo; lineo = temp; //swap the two buffer pointers "line old" and "line new"
				}
			}
			static unsigned long readBitFromReversedStream(size_t& bitp, const unsigned char* bits) { unsigned long result = (bits[bitp >> 3] >> (7 - (bitp & 0x7))) & 1; bitp++; return result; }
			static unsigned long readBitsFromReversedStream(size_t& bitp, const unsigned char* bits, unsigned long nbits)
			{
				unsigned long result = 0;
				for (size_t i = nbits - 1; i < nbits; i--) result += ((readBitFromReversedStream(bitp, bits)) << i);
				return result;
			}
			void setBitOfReversedStream(size_t& bitp, unsigned char* bits, unsigned long bit) { bits[bitp >> 3] |= (bit << (7 - (bitp & 0x7))); bitp++; }
			unsigned long read32bitInt(const unsigned char* buffer) { return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3]; }
			int checkColorValidity(unsigned long colorType, unsigned long bd) //return type is a LodePNG error code
			{
				if ((colorType == 2 || colorType == 4 || colorType == 6)) { if (!(bd == 8 || bd == 16)) return 37; else return 0; }
				else if (colorType == 0) { if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8 || bd == 16)) return 37; else return 0; }
				else if (colorType == 3) { if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8)) return 37; else return 0; }
				else return 31; //unexisting color type
			}
			unsigned long getBpp(const Info& info)
			{
				if (info.colorType == 2) return (3 * info.bitDepth);
				else if (info.colorType >= 4) return (info.colorType - 2) * info.bitDepth;
				else return info.bitDepth;
			}
			int convert(std::vector<unsigned char>& out, const unsigned char* in, Info& infoIn, unsigned long w, unsigned long h)
			{ //converts from any color type to 32-bit. return value = LodePNG error code
				size_t numpixels = w * h, bp = 0;
				out.resize(numpixels * 4);
				unsigned char* out_ = out.empty() ? 0 : &out[0]; //faster if compiled without optimization
				if (infoIn.bitDepth == 8 && infoIn.colorType == 0) //greyscale
					for (size_t i = 0; i < numpixels; i++)
					{
						out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[i];
						out_[4 * i + 3] = (infoIn.key_defined && in[i] == infoIn.key_r) ? 0 : 255;
					}
				else if (infoIn.bitDepth == 8 && infoIn.colorType == 2) //RGB color
					for (size_t i = 0; i < numpixels; i++)
					{
						for (size_t c = 0; c < 3; c++) out_[4 * i + c] = in[3 * i + c];
						out_[4 * i + 3] = (infoIn.key_defined == 1 && in[3 * i + 0] == infoIn.key_r && in[3 * i + 1] == infoIn.key_g && in[3 * i + 2] == infoIn.key_b) ? 0 : 255;
					}
				else if (infoIn.bitDepth == 8 && infoIn.colorType == 3) //indexed color (palette)
					for (size_t i = 0; i < numpixels; i++)
					{
						if (4U * in[i] >= infoIn.palette.size()) return 46;
						for (size_t c = 0; c < 4; c++) out_[4 * i + c] = infoIn.palette[4 * in[i] + c]; //get rgb colors from the palette
					}
				else if (infoIn.bitDepth == 8 && infoIn.colorType == 4) //greyscale with alpha
					for (size_t i = 0; i < numpixels; i++)
					{
						out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[2 * i + 0];
						out_[4 * i + 3] = in[2 * i + 1];
					}
				else if (infoIn.bitDepth == 8 && infoIn.colorType == 6) for (size_t i = 0; i < numpixels; i++) for (size_t c = 0; c < 4; c++) out_[4 * i + c] = in[4 * i + c]; //RGB with alpha
				else if (infoIn.bitDepth == 16 && infoIn.colorType == 0) //greyscale
					for (size_t i = 0; i < numpixels; i++)
					{
						out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[2 * i];
						out_[4 * i + 3] = (infoIn.key_defined && 256U * in[i] + in[i + 1] == infoIn.key_r) ? 0 : 255;
					}
				else if (infoIn.bitDepth == 16 && infoIn.colorType == 2) //RGB color
					for (size_t i = 0; i < numpixels; i++)
					{
						for (size_t c = 0; c < 3; c++) out_[4 * i + c] = in[6 * i + 2 * c];
						out_[4 * i + 3] = (infoIn.key_defined && 256U * in[6 * i + 0] + in[6 * i + 1] == infoIn.key_r && 256U * in[6 * i + 2] + in[6 * i + 3] == infoIn.key_g && 256U * in[6 * i + 4] + in[6 * i + 5] == infoIn.key_b) ? 0 : 255;
					}
				else if (infoIn.bitDepth == 16 && infoIn.colorType == 4) //greyscale with alpha
					for (size_t i = 0; i < numpixels; i++)
					{
						out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[4 * i]; //most significant byte
						out_[4 * i + 3] = in[4 * i + 2];
					}
				else if (infoIn.bitDepth == 16 && infoIn.colorType == 6) for (size_t i = 0; i < numpixels; i++) for (size_t c = 0; c < 4; c++) out_[4 * i + c] = in[8 * i + 2 * c]; //RGB with alpha
				else if (infoIn.bitDepth < 8 && infoIn.colorType == 0) //greyscale
					for (size_t i = 0; i < numpixels; i++)
					{
						unsigned long value = (readBitsFromReversedStream(bp, in, infoIn.bitDepth) * 255) / ((1 << infoIn.bitDepth) - 1); //scale value from 0 to 255
						out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = (unsigned char)(value);
						out_[4 * i + 3] = (infoIn.key_defined && value && ((1U << infoIn.bitDepth) - 1U) == infoIn.key_r && ((1U << infoIn.bitDepth) - 1U)) ? 0 : 255;
					}
				else if (infoIn.bitDepth < 8 && infoIn.colorType == 3) //palette
					for (size_t i = 0; i < numpixels; i++)
					{
						unsigned long value = readBitsFromReversedStream(bp, in, infoIn.bitDepth);
						if (4 * value >= infoIn.palette.size()) return 47;
						for (size_t c = 0; c < 4; c++) out_[4 * i + c] = infoIn.palette[4 * value + c]; //get rgb colors from the palette
					}
				return 0;
			}
			unsigned char paethPredictor(short a, short b, short c) //Paeth predicter, used by PNG filter type 4
			{
				short p = a + b - c, pa = p > a ? (p - a) : (a - p), pb = p > b ? (p - b) : (b - p), pc = p > c ? (p - c) : (c - p);
				return (unsigned char)((pa <= pb && pa <= pc) ? a : pb <= pc ? b : c);
			}
		};
		PNG decoder; decoder.decode(out_image, in_png, in_size, convert_to_rgba32);
		image_width = decoder.info.width; image_height = decoder.info.height;
		return decoder.error;
	}

	void suppressMissingImages(bool state)
	{
		if (global_suppressImages == true)
			global_suppressImages = false;
		else
			global_suppressImages = true;
	}

	void light(coords3d pos)
	{
		lightPos.x = pos.x * 0.01;
		lightPos.y = pos.y * 0.01;
		lightPos.z = pos.z * 0.01;
	}

	marker_type getMarker_main() // This returns the current size of the VBO allocation being used.
	{
		marker_type marker;

		if (dyn_mode == true)
		{
			marker.isStatic = false;
			marker.index = dyn_image_index;
			marker.index_type = 1;
		}
		else
		{
			marker.isStatic = true;
			marker.index = image_index;
			marker.index_type = 1;
		}

		return marker;
	}

	marker_type getMarker_ZC()
	{
		marker_type marker;

		if (dyn_mode == true)
		{
			marker.isStatic = false;
			marker.index = dyn_ZCimage_index;
			marker.index_type = 2;
		}
		else
		{
			marker.isStatic = true;
			marker.index = ZCimage_index;
			marker.index_type = 2;
		}

		return marker;
	}

	marker_type getMarker_HUD()
	{
		marker_type marker;

		if (dyn_mode == true)
		{
			marker.isStatic = false;
			marker.index = dyn_HUDimage_index;
			marker.index_type = 3;
		}
		else
		{
			marker.isStatic = true;
			marker.index = HUDimage_index;
			marker.index_type = 3;
		}

		return marker;
	}

	void gettextureDimensions(string filename, ipoint& pos, ipoint& dim)
	{
		multimap<string, int>::iterator it = atlas_map.find(filename);

		if (it == atlas_map.end())
		{
			if (failedtofind != filename)
			{
				cout << "Failed to find " << filename << " in atlas!\n";
				//failedtofind = filename;
				it = atlas_map.find("white0");
			}
			//it = atlas_map.find("black");
			//shutdown();
		}

		pos.x = atlas_x[it->second];
		pos.y = atlas_y[it->second];

		dim.x = atlas_xsize[it->second];
		dim.y = atlas_ysize[it->second];
	}

	point getimageDimensions(int handle)
	{
		return image_size[handle - 1];
	}

	point getHUDimageDimensions(int handle)
	{
		return HUDimage_size[handle - 1]; // We don't actually record this, look at older Iceberg where we capture it.
	}

	void pushVBO()
	{
		updateVBO = true;
	}

	void pushShaders()
	{
		updateShaders = true;
	}
	
	int handle(int event)
	{
		static int first = 1;
		if (first && event == FL_SHOW && shown())
		{
			first = 0;

			mode(FL_RGB8 | FL_DOUBLE | FL_ALPHA);
			make_current();

			int gi = glewInit();

			if (gi != GLEW_OK)
			{
				cout << "GLEW not ok\n";
				cout << "Glew error code = " << glewGetErrorString(gi) << "\n";
			}

			glEnable(GL_MULTISAMPLE);

			glDisable(GL_LIGHTING);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glClear(GL_ACCUM_BUFFER_BIT);

			setuplight();
			setupvbo();
			setupenvmap();
			loadatlas();
			setupshaders();

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glAlphaFunc(GL_GREATER, 0);	// enable this, for more jagged buy exactly transparent-on-transparent work 0.4
			glEnable(GL_ALPHA_TEST);

			glEnable(GL_CULL_FACE);

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);

			glBindBuffer(GL_ARRAY_BUFFER, vertex_id);
			glVertexPointer(3, GL_FLOAT, 0, 0);

			glBindBuffer(GL_ARRAY_BUFFER, normal_id);
			glNormalPointer(GL_FLOAT, 0, 0);

			glBindBuffer(GL_ARRAY_BUFFER, texcoord_id);
			glTexCoordPointer(4, GL_FLOAT, 0, 0);

			glBindBuffer(GL_ARRAY_BUFFER, color_id);
			glColorPointer(4, GL_FLOAT, 0, 0);

			// We disable client state when we quit.

			//Vertex attributes

			glBindBuffer(GL_ARRAY_BUFFER, att_PARASCAX_id);

			glEnableVertexAttribArray(PAlocation);
			glVertexAttribPointer(PAlocation, 4, GL_FLOAT, GL_FALSE, 0, 0);

			glEnableVertexAttribArray(RAlocation);
			glVertexAttribPointer(RAlocation, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(texcolor_array_limit * sizeof(GLfloat))); // 4 comp per vertex, 7500 quads x 4 = 30,000

			glEnableVertexAttribArray(SXlocation);
			glVertexAttribPointer(SXlocation, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET((texcolor_array_limit * 2) * sizeof(GLfloat))); // 4 comp per vertex, 7500 quads x 4 = 30,000

			glEnableVertexAttribArray(TDlocation);
			glVertexAttribPointer(TDlocation, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET((texcolor_array_limit * 3) * sizeof(GLfloat))); // 4 comp per vertex, 7500 quads x 4 = 30,000

			glEnableVertexAttribArray(MXlocation);
			glVertexAttribPointer(MXlocation, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET((texcolor_array_limit * 4) * sizeof(GLfloat))); // 4 comp per vertex, 7500 quads x 4 = 30,000

			glEnableVertexAttribArray(TXlocation);
			glVertexAttribPointer(TXlocation, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET((texcolor_array_limit * 5) * sizeof(GLfloat))); // 4 comp per vertex, 7500 quads x 4 = 30,000

			glBindTexture(GL_TEXTURE_CUBE_MAP, ENVtextureID);
			glBindTexture(GL_TEXTURE_2D, atlastextureID);

			redraw();
		}

		int ret = Fl_Group::handle(event);  // assume the buttons won't handle the keyboard events
		global_key = -1;
		//cout << "event = " << event << "\n";
		
		switch (event)
		{
			case FL_PUSH:
			{
				ret = 1;
			}
			case FL_RELEASE:            // keyboard key pushed
			{
				ret = 1;
			}
			case FL_SHORTCUT:            // keyboard key pushed
			{
				global_key = Fl::event_key();
				ret = 1;
				break;
			}
			case FL_KEYUP:              // keyboard key released
			{
				//global_key = Fl::event_key();
				ret = 1;
				break;
			}
			return(ret);
		}
	
		return Fl_Gl_Window::handle(event);
	}

	int newWorldToScreen()
	{
		f_result r = modifyWorldToScreen(true, false, false, 0, { 0.0f, 0.0f, 0.0f });

		if (r.success == false)
			return -1;
		else
			return r.i;
	}

	coords3d getWorldToScreen(int handle, coords3d worldpos)
	{
		f_result r = modifyWorldToScreen(false, false, true, handle, worldpos);

		if (r.success == true)
		{
			return r.c;
		}
		else
		{
			cout << "getWorldToScreen handle failed.\n";
			return { -100,-100, -100 };
		}
	}

	void deleteWorldToScreen(int handle)
	{
		f_result r = modifyWorldToScreen(false, true, false, handle, { 0.0f, 0.0f, 0.0f });
	}

	void cameras(coords3d _camera, coords3d _look, coords3d _cameraZC, coords3d _lookZC)
	{
		camera = _camera;
		look = _look;
		cameraZC = _cameraZC;
		lookZC = _lookZC;
	}

	void FixViewport(int W, int H)
	{
		glLoadIdentity();
		glViewport(0, 0, W, H);
		glOrtho(-W, W, -H, H, -1, 1);
	}

	/*
	void resize(int X, int Y, int W, int H)
	{
		Fl_Gl_Window::resize(X, Y, W, H);
		FixViewport(W, H);
		cout << "W = " << W << " H = " << H << "\n";
		xres = W;
		yres = H;
		redraw();
	}
	*/

	void redraw()
	{
		opengl_core();
		render(camera, look, cameraZC, lookZC);
	}

	void draw()
	{

		opengl_core();
		render(camera, look, cameraZC, lookZC);
	}

	coords3d mouseDepth() // ReadPixel depth in 3D
	{
		coords3d point;

		point.x = far_posX * 100.0;
		point.y = far_posY * 100.0;
		point.z = far_posZ * 100.0;

		return point;
	}

	coords3d mouseDepth_zc() // ReadPixel depth in 3D
	{
		coords3d point;

		point.x = far_posX_zc * 100.0;
		point.y = far_posY_zc * 100.0;
		point.z = far_posZ_zc * 100.0;

		return point;
	}

	void shutdown()
	{
		glDisableVertexAttribArray(PAlocation);
		glDisableVertexAttribArray(RAlocation);
		glDisableVertexAttribArray(SXlocation);
		glDisableVertexAttribArray(TDlocation);
		glDisableVertexAttribArray(MXlocation);
		glDisableVertexAttribArray(TXlocation);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		glDeleteBuffers(1, &vertex_id);
		glDeleteBuffers(1, &normal_id);
		glDeleteBuffers(1, &texcoord_id);
		glDeleteBuffers(1, &color_id);

		glDeleteBuffers(1, &att_PARASCAX_id);

		glDeleteTextures(1, &atlastextureID);
		glDeleteTextures(1, &ENVtextureID);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glDetachShader(program, v);
		glDetachShader(program, f);
		glDeleteShader(v);
		glDeleteShader(f);
		glDeleteProgram(program);

		exit(0);
	}

	~bluebush()
	{
		shutdown();
	}
};
