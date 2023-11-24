// I'd like to implement the capture of loaded ara dimensions for static physics items, but I don't think we normally catch that.
// The solution is something like, we catch the bounding for each ARA we load, and over-write it with the next one.
// bluebush_arena is anyway custom.

// BUG We need to complete OrthoTex and port objects to new format, otherwise, Scene loader breaks.
// It actually shouldn't break in the editor, even though in Production it would do that.



#define WIN32
#define NOMINMAX
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Roller.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Roller.H>
#include <FL/fl_message.H>
#include <FL/Fl_Gl_Window.H>
#include <gl/glew.h>
#include <gl/GLU.h>
#include <FL/gl.h>
#include "bluebush_arena.hpp"
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <filesystem>

using namespace std;

bluebush* gl;

stringstream ss;
string val;

bool operator==(const coords3d a, const coords3d b) // Equals of coords3d pair
{
	if (a.x == b.x && a.y == b.y && a.z == b.z)
		return true;
	else
		return false;
}

coords3d operator-(const coords3d a, const coords3d b) // Subtraction of coords3d pair
{
	coords3d r;

	r.x = a.x - b.x;
	r.y = a.y - b.y;
	r.z = a.z - b.z;

	return r;
}

coords3d operator*(const coords3d a, const coords3d b) // Multiply of coords3d pair
{
	coords3d r;

	r.x = a.x * b.x;
	r.y = a.y * b.y;
	r.z = a.z * b.z;

	return r;
}

coords3d operator*(const coords3d a, float s) // Multiply coords3d and float
{
	coords3d r;

	r.x = a.x * s;
	r.y = a.y * s;
	r.z = a.z * s;

	return r;
}

// Define a hash function for coords3d
struct coords3dHash
{
	std::size_t operator()(const coords3d& p) const 
	{
		// Use the hash values of the three float fields to create a hash value for the point
		std::size_t h1 = std::hash<float>()(p.x);
		std::size_t h2 = std::hash<float>()(p.y);
		std::size_t h3 = std::hash<float>()(p.z);
		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};

namespace std
{
	template <>
	struct hash<coords3d>
	{
		size_t operator()(const coords3d& p) const
		{
			// Use the hash values of the three float fields to create a hash value for the point
			std::size_t h1 = std::hash<float>()(p.x);
			std::size_t h2 = std::hash<float>()(p.y);
			std::size_t h3 = std::hash<float>()(p.z);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};
}

struct Quad
{
	string tex_name = "";
	coords3d v[4]; // Vertices
	coords3d n[4]; // Normals
	coords3d quad_normal = { 0.0f, 0.0f, 0.0f };
	ipoint t[4] = { {0,0},{0,0},{0,0},{0,0} };	// Texture coords
	int quad_handle = 0; // "image handle" returned by bluebush.
	int complete = 0;
};

struct Material
{
	bool faceted = false;
	float alpha = 1.0f;
	float fresnel = 2.0f;
	float envmap = 0.0f;
	string orthotex_image = "";
	float orthotex_Zoffset = 0;
	float orthotex_zoom = 1.0f;
	float orthotex_mixratio = 0.0f;
	bool reverse_winding = false;
};

struct Object
{
	vector <Quad> q;
	int quad_count = 0;
	Material m;
};

struct Transform
{
	coords3d translate = { 0.0f, 0.0f, 0.0f};
	coords3d rotate = { 0.0f, 0.0f, 0.0f };
	coords3d scale = { 1.0f, 1.0f, 1.0f };
};

struct Scene
{
	string object_file = "";
	string object_path = "";
	coords3d translate = { 0.0f, 0.0f, 0.0f };
	coords3d rotate = { 0.0f, 0.0f, 0.0f };
	float scale = 1.0f;
};

struct CubemapPair
{
	string name = "";
	string path = "";
};

Object ara_original;
Object ara_transformed;

unordered_map<coords3d, coords3d, coords3dHash> unique_verts;

marker_type EmptyObject;
marker_type EmptyScene;

vector <Scene> S;
coords3d scnGlobalPos = { 0.0f, 0.0f, 0.0f };

bool obj_mode = true;
coords3d cameraPos;
coords3d lookPos;
coords3d ZCcameraPos;
coords3d ZClookPos;
bool cameraOn = false;
float cameraSpeed = 250.0f;

int lastNormalsCalced = 0; // 1 Average, 2 Faceted.

struct VertexTuple
{
	coords3d vertex;
	point texcoord;
	coords3d normal;
};

Fl_Text_Display* quadCount_display;
Fl_Text_Buffer* quadCountbuff;
Fl_Menu_Bar* menu;
Fl_Light_Button* light_facet_button;
Fl_Light_Button* light_average_button;
Fl_Button* camera_button;
Fl_Roller* aroller;
Fl_Native_File_Chooser* file_chooser;
Fl_Tabs* mtabs1;
Fl_Tabs* tabs;
Fl_Group* obj_group;
Fl_Group* scn_group;
Fl_Group* cam_group;
Fl_Group* shaders_group;

Fl_Float_Input* xpos_counter;
Fl_Float_Input* ypos_counter;
Fl_Float_Input* zpos_counter;
Fl_Float_Input* xrot_counter;
Fl_Float_Input* yrot_counter;
Fl_Float_Input* zrot_counter;
Fl_Float_Input* xscale_counter;
Fl_Float_Input* yscale_counter;
Fl_Float_Input* zscale_counter;
Fl_Value_Slider* fresnel_slider;
Fl_Value_Slider* envmap_slider;
Fl_Value_Slider* alpha_slider;
Fl_Check_Button* winding_checkbox;
Fl_Check_Button* maintexOffset_checkbox;
Fl_Float_Input* maintexOffset_counter;
Fl_Text_Display* bounds_display;
Fl_Text_Buffer* textbuff;
Fl_Text_Display* camera_display;
Fl_Text_Buffer* camerabuff;

Fl_Hold_Browser* quadBrowser;
Fl_Spinner* quadPinch_spinner;
Fl_Check_Button* quadToggle_checkbox;
Fl_Value_Slider* quadRotate_slider;

Fl_Hold_Browser* imgbrowser;
Fl_Float_Input* zoffset_counter;
Fl_Roller* zoffset_roller;
Fl_Float_Input* zoom_counter;
Fl_Value_Slider* mixratio_slider;

unordered_set<string> current_textures;
Fl_Hold_Browser* current_browser;
Fl_Hold_Browser* replacement_browser;
Fl_Hold_Browser* atlas_browser;
Fl_Button* transferdown_button;
Fl_Button* transferup_button;
Fl_Button* cleartex_button;
Fl_Button* replace_button;

Fl_Hold_Browser* scene_browser;
Fl_Button* object_browse_button;
Fl_Button* object_delete_button;
Fl_Float_Input* scn_posx_counter;
Fl_Float_Input* scn_posy_counter;
Fl_Float_Input* scn_posz_counter;
Fl_Float_Input* scn_rotx_counter;
Fl_Float_Input* scn_roty_counter;
Fl_Float_Input* scn_rotz_counter;
Fl_Float_Input* scn_scale_counter;
Fl_Button* scn_up_button;
Fl_Button* scn_down_button;
Fl_Float_Input* scn_gblx_counter;
Fl_Float_Input* scn_gbly_counter;
Fl_Float_Input* scn_gblz_counter;
Fl_Choice* scn_shape_dropdown;
Fl_Check_Button* scn_physics_checkbox;

Fl_Hold_Browser* cubemap_browser;
Fl_Button* scn_cubemap_button;
Fl_Button* scn_cubemap_delete_button;
Fl_Input* cubemapInput;
vector<CubemapPair> cubemapData;

Fl_Hold_Browser* wavefront_browser;
string wavefront_reference = "white0";
Fl_Check_Button* intex_checkbox;
Fl_Float_Input* pinch_counter;
int pinch_int = 1;
bool intex_bool = false;

Fl_Check_Button* ortho_checkbox;
Fl_Check_Button* backface_checkbox;
Fl_Check_Button* wireframe_checkbox;
Fl_Check_Button* originx_checkbox;
Fl_Check_Button* originy_checkbox;
Fl_Check_Button* originz_checkbox;
Fl_Value_Slider* origin_slider;
Fl_Float_Input* camera_counter;

Fl_Text_Editor* vertex_editor;
Fl_Text_Editor* fragment_editor;
Fl_Text_Buffer* vertexbuff;
Fl_Text_Buffer* fragmentbuff;
Fl_Button* vertex_reset_button;
Fl_Button* vertex_compile_button;
Fl_Button* vertex_save_button;
Fl_Button* fragment_reset_button;
Fl_Button* fragment_compile_button;
Fl_Button* fragment_save_button;

Fl_Hold_Browser* rail_browser;
Fl_Float_Input* railwidth_counter;
Fl_Float_Input* railgauge_counter;
Fl_Float_Input* railsteps_counter;
Fl_Button* railsave_button;

coords3d minBound = { 0.0f, 0.0f, 0.0f };
coords3d maxBound = { 0.0f, 0.0f, 0.0f };

vector <coords3d> vertex_list;
vector <point> texcoord_list;

size_t scnAfterLoadHash = 0;
size_t scnCurrentHash = 0;

string initial_ara = "";

bool mouse_click = false;

float rounding(float var)
{
	// 37.66666 * 100 =3766.66
	// 3766.66 + .5 =3767.16    for rounding off value
	// then type cast to int so value is 3767
	// then divided by 100 so the value converted into 37.67
	float value = (int)(var * 100 + .5);
	return (float)value / 100;
}

inline string to_trimmedString(float f)
{
	return to_string(f).substr(0, std::to_string(f).find(".") + 2 + 1);
}

// Hash function for strings
size_t stringHash(const std::string& s)
{
	size_t h = 0;
	for (char c : s) {
		h = h * 31 + c;
	}
	return h;
}

void getPathAndFilename(string fullpath, string& pathonly, string& fileonly)
{	
	bool foundFile = false;

	for (int i = fullpath.size() - 1; i > -1; i--) // Go backwards to find the slash
	{
		if (fullpath[i] != '\\' && foundFile == false)
			fileonly = fileonly + fullpath[i];
		else
		{
			foundFile = true;
			pathonly = pathonly + fullpath[i];
		}
	}

	string ffn;

	for (int i = fileonly.size() - 1; i > -1; i--) // Reverse the filename again.
		ffn = ffn + fileonly[i];

	string ffp;

	for (int i = pathonly.size() - 1; i > -1; i--) // Reverse the filepath again.
		ffp = ffp + pathonly[i];

	fileonly = ffn;
	pathonly = ffp;
}

size_t getSceneHash()
{
	string sceneString = "";

	for (int i = 0; i < S.size(); i++)
		sceneString = sceneString + S[i].object_file + S[i].object_path + to_string(S[i].translate.x) + to_string(S[i].translate.y) + to_string(S[i].translate.z) +
		to_string(S[i].rotate.x) + to_string(S[i].rotate.y) + to_string(S[i].rotate.z) + to_string(S[i].scale);

	return stringHash(sceneString);
}

void calc_quad_normals()
{
	tri triangle1;
	tri triangle2;

	for (int i = 0; i < ara_original.quad_count; i++) // Calc faceted normals
	{
		if (ara_transformed.m.reverse_winding == false)
		{
			triangle1.a = ara_transformed.q[i].v[1];  // forward winding ('trick' the lighting, because -1 is up for us.)
			triangle1.b = ara_transformed.q[i].v[3];
			triangle1.c = ara_transformed.q[i].v[0];

			triangle2.a = ara_transformed.q[i].v[1];
			triangle2.b = ara_transformed.q[i].v[2];
			triangle2.c = ara_transformed.q[i].v[3];
		}
		else if (ara_transformed.m.reverse_winding == true)
		{
			triangle1.a = ara_transformed.q[i].v[1];  // reverse winding
			triangle1.b = ara_transformed.q[i].v[0];
			triangle1.c = ara_transformed.q[i].v[3];

			triangle2.a = ara_transformed.q[i].v[1];
			triangle2.b = ara_transformed.q[i].v[3];
			triangle2.c = ara_transformed.q[i].v[2];
		}

		coords3d tri_normal1 = gl->calc_triangle_normal(triangle1);
		coords3d tri_normal2 = gl->calc_triangle_normal(triangle2);

		coords3d sum = tri_normal1;

		sum = sum + tri_normal2;

		sum = gl->normalize(sum);

		ara_transformed.q[i].quad_normal = sum; // Average normals calc can use this field.
		ara_original.q[i].quad_normal = sum;

		ara_transformed.q[i].n[0] = sum; // Faceted by default.
		ara_transformed.q[i].n[1] = sum;
		ara_transformed.q[i].n[2] = sum;
		ara_transformed.q[i].n[3] = sum;

		ara_original.q[i].n[0] = sum;
		ara_original.q[i].n[1] = sum;
		ara_original.q[i].n[2] = sum;
		ara_original.q[i].n[3] = sum;
	}
}

void generate_average_normals()
{
	unordered_map<coords3d, vector<int>> adjacentMap;

	// Collect unique verts and add a list to each one of the quad index they are used in. These are the adjacent vertices.

	for (int i = 0; i < ara_transformed.quad_count; i++) // Step through each quad.
		for (int j = 0; j < 4; j++) // Check the four corners of the quad.
			adjacentMap[ara_transformed.q[i].v[j]].push_back(i); // Map this vertex to this Quad index.

	map<coords3d, coords3d> vertexNormalMap; // Where we store our vertices -> normal calculations.

	// Iterate through all the vertex keys and process the list of quad indices

	for (const std::pair<coords3d, std::vector<int>>& entry : adjacentMap)
	{
		const coords3d& key = entry.first;
		const std::vector<int>& value = entry.second;

		coords3d resultant = { 0.0f, 0.0f, 0.0f };

		for (int val : value)
			resultant = resultant + ara_transformed.q[val].quad_normal;

		resultant = gl->normalize(resultant);
		vertexNormalMap[key] = resultant;
	}

	// Iterate of ara_transformed and search the map for each vertex and assign the new per-vertex resultant.

	for (int i = 0 ; i < ara_transformed.quad_count ; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			std::map<coords3d, coords3d>::iterator iter = vertexNormalMap.find(ara_transformed.q[i].v[j]);
			if (iter != vertexNormalMap.end())
			{
				const coords3d& key = iter->first;
				const coords3d& value = iter->second;
				ara_transformed.q[i].n[j] = value;
				ara_original.q[i].n[j] = value;
			}
			else
				std::cout << " !!! Key not found in the map. !!!" << std::endl;
		}
	}
}

void global_transformObject() // Calculate ara_transformed, No Bounds generation, no object reconstruction, it will generate normals without blocking.
{
	if (ara_original.quad_count > 0)
	{
		Transform t;

		if (xpos_counter->size() > 0 && ypos_counter->size() > 0 && zpos_counter->size() > 0)
			t.translate = { stof(xpos_counter->value()), stof(ypos_counter->value()), stof(zpos_counter->value()) };
		if (xrot_counter->size() > 0 && yrot_counter->size() > 0 && zrot_counter->size() > 0)
			t.rotate = { stof(xrot_counter->value()), stof(yrot_counter->value()), stof(zrot_counter->value()) };
		if (xscale_counter->size() > 0 && yscale_counter->size() > 0 && zscale_counter->size() > 0)
			t.scale = { stof(xscale_counter->value()), stof(yscale_counter->value()), stof(zscale_counter->value()) };

		Material m;
		m.alpha = alpha_slider->value();
		m.envmap = envmap_slider->value();
		m.fresnel = fresnel_slider->value();

		if (light_facet_button->value() == 1)
		{
			m.faceted = true;
			ara_transformed.m.faceted = true;
		}
		else if (light_average_button->value() == 1)
		{
			m.faceted = false;
			ara_transformed.m.faceted = false;
		}

		if (winding_checkbox->value() == 1)
			m.reverse_winding = true;
		else
			m.reverse_winding = false;

		if (imgbrowser->value() > 0) // Something was selected.
		{ 
			m.orthotex_image = imgbrowser->text(imgbrowser->value());

			if (zoffset_roller->changed() != 0  ) // If roller changed value, over-write zoffset counter.
			{
				m.orthotex_Zoffset = zoffset_roller->value();
				zoffset_counter->value(to_trimmedString(zoffset_roller->value()).c_str());
			}
			else
				m.orthotex_Zoffset = atoi(zoffset_counter->value());

			m.orthotex_zoom = stof(zoom_counter->value());
			m.orthotex_mixratio = mixratio_slider->value();
		}

		ara_transformed.m = m;
		ara_transformed.quad_count = ara_original.quad_count;

		Object ara_previous = ara_transformed; // Hang on to previously calculated normals.
		ara_transformed.q.clear();

		Quad q;

		current_textures.clear();
		replacement_browser->clear();

		for (int i = 0; i < ara_original.quad_count; i++)
		{
			q.tex_name = ara_original.q[i].tex_name;
			current_textures.insert(q.tex_name); // For the texture replacement function
			q.complete = 0;

			for (int j = 0; j < 4; j++)
			{
				q.v[j] = ara_original.q[i].v[j] + t.translate; // First, translate
				q.v[j] = gl->transform(q.v[j], t.rotate); // Rotate
				q.v[j] = q.v[j] * t.scale; // Scale
				q.t[j] = ara_original.q[i].t[j]; // Just move the tex coords across
			}

			ara_transformed.q.emplace_back(q);
		}

		current_browser->clear();
		for ( auto i : current_textures )
			current_browser->add(i.c_str());

		if (ara_transformed.m.faceted == false)
		{
			calc_quad_normals();
			generate_average_normals();
		}
		else if (ara_transformed.m.faceted == true)
		{
			calc_quad_normals();
		}
		else
			{
				// Pass the normals as they are.

				for (int i = 0; i < ara_original.quad_count; i++)
					for (int j = 0; j < 4; j++)
						ara_transformed.q[i].n[j] = ara_previous.q[i].n[j];
			}
	}
}

void loading_ice_file(string filename)
{
	//ara_original = Object(); // Reset all values of the object.
	//ara_transformed = Object();

	string line;
	ifstream versionfile(filename);

	int version_format = 0;

	if (versionfile.is_open())
	{
		std::getline(versionfile, line); // Version check
		int fileversion = atoi(line.c_str());

		if (fileversion < 1104)
		{
			cout << "Obsolete format detected - not loading.\n";
			versionfile.close();
		}
		else if (fileversion == 1104)
		{
			cout << "1.4 format detected.\n";
			version_format = 5;
			versionfile.close();
		}
	}

	if (version_format == 5)
	{
		ifstream objfile(filename);

		minBound = { 0.0f, 0.0f, 0.0f };
		maxBound = { 0.0f, 0.0f, 0.0f };

		if (objfile.is_open())
		{
			getline(objfile, line); // Version line.

			getline(objfile, line); // Manifold count
			ara_original.quad_count = atoi(line.c_str());
			string qtb = "Quads: " + to_string(ara_original.quad_count);
			quadCountbuff->text(qtb.c_str());

			cout << "ICE file has " << ara_original.quad_count << " manifolds.\n";

			ara_original.q.clear();

			for (int i = 0; i < ara_original.quad_count; i++)
			{
				getline(objfile, line); // Group ID (We ignore this)

				Quad q;

				getline(objfile, q.tex_name); // Fast string to int.

				getline(objfile, line);
				q.v[0].x = stof(line.c_str());
				getline(objfile, line);
				q.v[0].y = stof(line.c_str());
				getline(objfile, line);
				q.v[0].z = stof(line.c_str());

				getline(objfile, line);
				q.v[1].x = stof(line.c_str());
				getline(objfile, line);
				q.v[1].y = stof(line.c_str());
				getline(objfile, line);
				q.v[1].z = stof(line.c_str());

				getline(objfile, line);
				q.v[2].x = stof(line.c_str());
				getline(objfile, line);
				q.v[2].y = stof(line.c_str());
				getline(objfile, line);
				q.v[2].z = stof(line.c_str());

				getline(objfile, line);
				q.v[3].x = stof(line.c_str());
				getline(objfile, line);
				q.v[3].y = stof(line.c_str());
				getline(objfile, line);
				q.v[3].z = stof(line.c_str());

				// Determine minimum boundary.
				if (q.v[0].x < minBound.x)
					minBound.x = q.v[0].x;
				if (q.v[1].x < minBound.x)
					minBound.x = q.v[1].x;
				if (q.v[2].x < minBound.x)
					minBound.x = q.v[2].x;
				if (q.v[3].x < minBound.x)
					minBound.x = q.v[3].x;

				if (q.v[0].y < minBound.y)
					minBound.y = q.v[0].y;
				if (q.v[1].y < minBound.y)
					minBound.y = q.v[1].y;
				if (q.v[2].y < minBound.y)
					minBound.y = q.v[2].y;
				if (q.v[3].y < minBound.y)
					minBound.y = q.v[3].y;

				if (q.v[0].z < minBound.z)
					minBound.z = q.v[0].z;
				if (q.v[1].z < minBound.z)
					minBound.z = q.v[1].z;
				if (q.v[2].z < minBound.z)
					minBound.z = q.v[2].z;
				if (q.v[3].z < minBound.z)
					minBound.z = q.v[3].z;

				// Determine maximum boundary.
				if (q.v[0].x > maxBound.x)
					maxBound.x = q.v[0].x;
				if (q.v[1].x > maxBound.x)
					maxBound.x = q.v[1].x;
				if (q.v[2].x > maxBound.x)
					maxBound.x = q.v[2].x;
				if (q.v[3].x > maxBound.x)
					maxBound.x = q.v[3].x;

				if (q.v[0].y > maxBound.y)
					maxBound.y = q.v[0].y;
				if (q.v[1].y > maxBound.y)
					maxBound.y = q.v[1].y;
				if (q.v[2].y > maxBound.y)
					maxBound.y = q.v[2].y;
				if (q.v[3].y > maxBound.y)
					maxBound.y = q.v[3].y;

				if (q.v[0].z > maxBound.z)
					maxBound.z = q.v[0].z;
				if (q.v[1].z > maxBound.z)
					maxBound.z = q.v[1].z;
				if (q.v[2].z > maxBound.z)
					maxBound.z = q.v[2].z;
				if (q.v[3].z > maxBound.z)
					maxBound.z = q.v[3].z;

				// Vertex Color RGB (ARA ignores vertex colors)

				for ( int j = 0 ; j < 12 ; j++ ) // Quickly skip through the vertex color values..
					getline(objfile, line);

				// Tex Coords

				getline(objfile, line);
				q.t[0].x = stof(line.c_str());
				getline(objfile, line);
				q.t[0].y = stof(line.c_str());

				getline(objfile, line);
				q.t[1].x = stof(line.c_str());
				getline(objfile, line);
				q.t[1].y = stof(line.c_str());

				getline(objfile, line);
				q.t[2].x = stof(line.c_str());
				getline(objfile, line);
				q.t[2].y = stof(line.c_str());

				getline(objfile, line);
				q.t[3].x = stof(line.c_str());
				getline(objfile, line);
				q.t[3].y = stof(line.c_str());

				// Normals loaded

				getline(objfile, line);
				q.n[0].x = stof(line.c_str());
				getline(objfile, line);
				q.n[0].y = stof(line.c_str());
				getline(objfile, line);
				q.n[0].z = stof(line.c_str());

				getline(objfile, line);
				q.n[1].x = stof(line.c_str());
				getline(objfile, line);
				q.n[1].y = stof(line.c_str());
				getline(objfile, line);
				q.n[1].z = stof(line.c_str());

				getline(objfile, line);
				q.n[2].x = stof(line.c_str());
				getline(objfile, line);
				q.n[2].y = stof(line.c_str());
				getline(objfile, line);
				q.n[2].z = stof(line.c_str());

				getline(objfile, line);
				q.n[3].x = stof(line.c_str());
				getline(objfile, line);
				q.n[3].y = stof(line.c_str());
				getline(objfile, line);
				q.n[3].z = stof(line.c_str());

				getline(objfile, line);
				ara_original.m.alpha = stof(line.c_str()); // Alpha is appended here for legacy reasons, it seems.
				cout << "ICE alpha for manifold = " << ara_original.m.alpha << "\n";

				// .Reverse flag

				getline(objfile, line);
				int reverse_int = atoi(line.c_str());

				getline(objfile, line);
				ara_original.m.fresnel = stof(line.c_str());
				getline(objfile, line);
				ara_original.m.envmap = stof(line.c_str());

				if (reverse_int == 0)
					ara_original.m.reverse_winding = false;
				else if (reverse_int == 1)
					ara_original.m.reverse_winding = true;

				coords3d col = { 1.0f, 1.0f, 1.0f }; // We default vertex color to "white"

				if (ara_original.m.reverse_winding == false)
				{
					q.quad_handle = gl->loadimage_patch(q.tex_name, q.v[0], q.v[1], q.v[2], q.v[3], col, col, col, col, q.t[0], q.t[1], q.t[2], q.t[3], q.n[0], q.n[1], q.n[2], q.n[3], ara_original.m.fresnel, ara_original.m.envmap, ara_original.m.alpha);
					cout<<"ICE patch " <<q.quad_handle<<"\n";
				}
				else if (ara_original.m.reverse_winding == true)
				{
					q.quad_handle = gl->loadimage_patch_r(q.tex_name, q.v[0], q.v[1], q.v[2], q.v[3], col, col, col, col, q.t[0], q.t[1], q.t[2], q.t[3], q.n[0], q.n[1], q.n[2], q.n[3], ara_original.m.fresnel, ara_original.m.envmap, ara_original.m.alpha);
					cout << "ICE patch_r " << q.quad_handle << "\n";
				}

				gl->glassimage(q.quad_handle, 1.0);

				ara_original.q.emplace_back(q);
			}

			// Set the widgets with the properties of this ICE object.

			xpos_counter->value("0.0");
			ypos_counter->value("0.0");
			zpos_counter->value("0.0");
			xscale_counter->value("1.0");
			yscale_counter->value("1.0");
			zscale_counter->value("1.0");
			xrot_counter->value("0.0");
			yrot_counter->value("0.0");
			zrot_counter->value("0.0");

			envmap_slider->value(ara_original.m.envmap);
			fresnel_slider->value(ara_original.m.fresnel);
			alpha_slider->value(ara_original.m.alpha);

			mixratio_slider->value(0.0);  	// ICE doesn't support these properties, so we turn them off on the UI.
			zoffset_counter->value("0.00");
			zoffset_roller->value(0.00);
			zoom_counter->value("0.00");
			imgbrowser->value(0);

			if (ara_original.m.reverse_winding == true)
				winding_checkbox->value(1);
			if (ara_original.m.reverse_winding == false)
				winding_checkbox->value(0);

			ara_original.m.orthotex_image = "";
			ara_original.m.orthotex_Zoffset = 0.0f;
			ara_original.m.orthotex_mixratio = 0;
			ara_original.m.orthotex_zoom = 0.0f;

			coords3d dims = { maxBound.x - minBound.x, maxBound.y - minBound.y, maxBound.z - minBound.z };
			string tb = "Dimensions: " + to_trimmedString(dims.x) + ", " + to_trimmedString(dims.y) + ", " + to_trimmedString(dims.z) + ", Spatial: min(" + to_trimmedString(minBound.x) + ", " + to_trimmedString(minBound.y) + ", " + to_trimmedString(minBound.z) + ") max(" + to_trimmedString(maxBound.x) + ", " + to_trimmedString(maxBound.y) + ", " + to_trimmedString(maxBound.z) + ")";				
			textbuff->text(tb.c_str());
	
			objfile.close();

			global_transformObject();
			Fl::redraw();
		}
	}
}

void saving_scn_file(string filename)
{
	cout << "Saving SCN specification.\n";

	ofstream SCNfile(filename, ios::out | ios::binary);
	if (!SCNfile)
	{
		cout << "ARENA cannot for some reason save the SCN file.\n";
		return;
	}

	string version = "1.00";
	string::size_type sz = version.size(); // Get string size
	SCNfile.write(reinterpret_cast<char*>(&sz), sizeof(string::size_type)); // Write string size
	SCNfile.write(version.data(), sz); // Write string

	int c = S.size(); // Object count
	SCNfile.write(reinterpret_cast<char*>(&c), sizeof(int));

	for (int i = 0; i < S.size(); i++)
	{
		sz = S[i].object_file.size(); // Get string size
		SCNfile.write(reinterpret_cast<char*>(&sz), sizeof(string::size_type)); // Write string size
		SCNfile.write(S[i].object_file.data(), sz); // Write string

		coords3d ttranslate = { S[i].translate.x + scnGlobalPos.x, S[i].translate.y + scnGlobalPos.y, S[i].translate.z + scnGlobalPos.z };

		SCNfile.write(reinterpret_cast<char*>(&ttranslate.x), sizeof(float));
		SCNfile.write(reinterpret_cast<char*>(&ttranslate.y), sizeof(float));
		SCNfile.write(reinterpret_cast<char*>(&ttranslate.z), sizeof(float));

		SCNfile.write(reinterpret_cast<char*>(&S[i].rotate.x), sizeof(float));
		SCNfile.write(reinterpret_cast<char*>(&S[i].rotate.y), sizeof(float));
		SCNfile.write(reinterpret_cast<char*>(&S[i].rotate.z), sizeof(float));

		SCNfile.write(reinterpret_cast<char*>(&S[i].scale), sizeof(float));
	}

	SCNfile.close();

	scnAfterLoadHash = getSceneHash(); // "AfterLoad" is a bit of a misnomer in this case, but it's required to align them.
	scnCurrentHash = scnAfterLoadHash;
}

void loading_scn_file(string filename)
{
	cout << "Loading SCN.\n";

	// Presumption: The ARA files reside in the same path as this SCN file.

	string rfn; // File name
	string rfp; // File path
	bool foundFile = false;

	for (int i = filename.size() - 1; i > -1; i--) // Go backwards to find the slash
	{
		if (filename[i] != '\\' && foundFile == false)
			rfn = rfn + filename[i];
		else
		{
			foundFile = true;
			rfp = rfp + filename[i];
		}
	}

	string ffn;

	for (int i = rfn.size() - 1; i > -1; i--) // Reverse the filename again.
		ffn = ffn + rfn[i];

	string ffp;

	for (int i = rfp.size() - 1; i > -1; i--) // Reverse the filepath again.
		ffp = ffp + rfp[i];

	//cout << "Path: " << ffp << "\n";
	//cout << "File: " << ffn << "\n";

	ifstream SCNfile;
	SCNfile.open(filename, ios::binary);

	string::size_type sz;

	SCNfile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
	string version;
	version.resize(sz);
	SCNfile.read(&version[0], sz);

	int SCNsize = 0;
	SCNfile.read(reinterpret_cast<char*>(&SCNsize), sizeof(int));

	Scene s;

	scene_browser->clear();

	gl->suppressMissingImages(true);

	for (int i = 0; i < SCNsize; i++)
	{
		SCNfile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
		s.object_file.resize(sz);
		SCNfile.read(&s.object_file[0], sz);

		s.object_path = ffp;

		SCNfile.read(reinterpret_cast<char*>(&s.translate.x), sizeof(float));
		SCNfile.read(reinterpret_cast<char*>(&s.translate.y), sizeof(float));
		SCNfile.read(reinterpret_cast<char*>(&s.translate.z), sizeof(float));

		SCNfile.read(reinterpret_cast<char*>(&s.rotate.x), sizeof(float));
		SCNfile.read(reinterpret_cast<char*>(&s.rotate.y), sizeof(float));
		SCNfile.read(reinterpret_cast<char*>(&s.rotate.z), sizeof(float));

		SCNfile.read(reinterpret_cast<char*>(&s.scale), sizeof(float));

		cout << "  SCN object = " << s.object_file << "\n";
		int o = gl->loadZCobject(s.object_path + s.object_file);
		if (o != -1)
		{
			gl->moveZCobject(o, s.translate);
			gl->rotateZCobject(o, s.rotate);
			gl->scaleZCobject(o, s.scale);
			gl->glassZCobject(o, 1.0f);

			S.emplace_back(s);
			scene_browser->add(s.object_file.c_str());
		}
	}

	gl->suppressMissingImages(false);

	SCNfile.close();

	marker_type sceneCount = gl->getMarker_ZC();
	string qtb = "Scene: " + to_string(sceneCount.index);
	quadCountbuff->text(qtb.c_str());

	scnAfterLoadHash = getSceneHash();
	scnCurrentHash = scnAfterLoadHash;
}

void saving_ara_file(string filename)
{
	cout << "Saving ARA specification.\n";

	global_transformObject(); // Generate updated state of ara_transformed.

	ofstream ARAfile(filename, ios::out | ios::binary);
	if (!ARAfile)
	{ 
		cout << "ARENA cannot for some reason save the ARA file.\n";
		return;
	}

	string version = "1.01";
	string::size_type sz = version.size(); // Get string size
	ARAfile.write(reinterpret_cast<char*>(&sz), sizeof(string::size_type)); // Write string size
	ARAfile.write(version.data(), sz); // Write string

	ARAfile.write(reinterpret_cast<char*>(&ara_transformed.quad_count), sizeof(int));

	if (ara_transformed.m.reverse_winding == false)
	{
		int number = 0;
		ARAfile.write(reinterpret_cast<char*>(&number), sizeof(int));
	}
	else if (ara_transformed.m.reverse_winding == true)
	{
		int number = 1;
		ARAfile.write(reinterpret_cast<char*>(&number), sizeof(int));
	}

	ARAfile.write(reinterpret_cast<char*>(&ara_transformed.m.fresnel), sizeof(float));
	ARAfile.write(reinterpret_cast<char*>(&ara_transformed.m.envmap), sizeof(float));
	ARAfile.write(reinterpret_cast<char*>(&ara_transformed.m.alpha), sizeof(float));

	sz = ara_transformed.m.orthotex_image.size(); // Get string size
	ARAfile.write(reinterpret_cast<char*>(&sz), sizeof(string::size_type)); // Write string size
	ARAfile.write(ara_transformed.m.orthotex_image.data(), sz); // Write string

	ARAfile.write(reinterpret_cast<char*>(&ara_transformed.m.orthotex_zoom), sizeof(float));
	ARAfile.write(reinterpret_cast<char*>(&ara_transformed.m.orthotex_mixratio), sizeof(float));
	ARAfile.write(reinterpret_cast<char*>(&ara_transformed.m.orthotex_Zoffset), sizeof(float));
	
	for (int i = 0 ; i < ara_transformed.quad_count ; i++)
	{
		sz = ara_transformed.q[i].tex_name.size(); // Get string size
		ARAfile.write(reinterpret_cast<char*>(&sz), sizeof(string::size_type)); // Write string size
		ARAfile.write(ara_transformed.q[i].tex_name.data(), sz); // Write string

		for (int j = 0; j < 4; j++)
		{
			ARAfile.write(reinterpret_cast<char*>(&ara_transformed.q[i].v[j].x), sizeof(float));
			ARAfile.write(reinterpret_cast<char*>(&ara_transformed.q[i].v[j].y), sizeof(float));
			ARAfile.write(reinterpret_cast<char*>(&ara_transformed.q[i].v[j].z), sizeof(float));
		}


		if (maintexOffset_checkbox->value() == 1) // Use the over-ride texcoords in Materials dialog instead.
		{
			ipoint ta = { 1,1 }; // Default values, in case the counter doesn't have a valid value.
			ipoint tb = { -1,1 };
			ipoint tc = { -1,-1 };
			ipoint td = { 1,-1 };

			if (maintexOffset_counter->size() > 0)
			{
				int tv = atoi(maintexOffset_counter->value());
				ta = { tv, tv };
				tb = { -tv, tv };
				tc = { -tv, -tv };
				td = { tv, -tv };
			}

			ARAfile.write(reinterpret_cast<char*>(&ta.x), sizeof(float));
			ARAfile.write(reinterpret_cast<char*>(&ta.y), sizeof(float));

			ARAfile.write(reinterpret_cast<char*>(&tb.x), sizeof(float));
			ARAfile.write(reinterpret_cast<char*>(&tb.y), sizeof(float));

			ARAfile.write(reinterpret_cast<char*>(&tc.x), sizeof(float));
			ARAfile.write(reinterpret_cast<char*>(&tc.y), sizeof(float));

			ARAfile.write(reinterpret_cast<char*>(&td.x), sizeof(float));
			ARAfile.write(reinterpret_cast<char*>(&td.y), sizeof(float));
		}
		else
		{
			for (int j = 0; j < 4; j++) // Regular texcoords, including the ones imported by OBJ.
			{
				ARAfile.write(reinterpret_cast<char*>(&ara_transformed.q[i].t[j].x), sizeof(float));
				ARAfile.write(reinterpret_cast<char*>(&ara_transformed.q[i].t[j].y), sizeof(float));
			}
		}

		for (int j = 0; j < 4; j++)
		{
			ARAfile.write(reinterpret_cast<char*>(&ara_transformed.q[i].n[j].x), sizeof(float));
			ARAfile.write(reinterpret_cast<char*>(&ara_transformed.q[i].n[j].y), sizeof(float));
			ARAfile.write(reinterpret_cast<char*>(&ara_transformed.q[i].n[j].z), sizeof(float));
		}
	}

	ARAfile.close();
}

void exporting_obj_file(string filename)
{
	cout << "Exporting to Wavefront OBJ.\n";

	global_transformObject(); // Generate updated state of ara_transformed.

	vector <coords3d> originalVerts; // This is a parallel structure to ara_transformed.q[].v[]
	Object ara_scaled;
	ara_scaled = ara_transformed;

	for (int i = 0 ; i < ara_scaled.quad_count ; i++) // Scaled version of ara_transformed.
		for (int j = 0; j < 4; j++)
		{
			if (ara_scaled.q[i].v[j].x != 0.0f) // Avoid dividing by zero.
				ara_scaled.q[i].v[j].x = ara_scaled.q[i].v[j].x / 150.0f; // Scaling it back down for export.
			if (ara_scaled.q[i].v[j].y != 0.0f)
				ara_scaled.q[i].v[j].y = ara_scaled.q[i].v[j].y / 150.0f;
			if (ara_scaled.q[i].v[j].z != 0.0f)
				ara_scaled.q[i].v[j].z = ara_scaled.q[i].v[j].z / 150.0f;

			originalVerts.push_back(ara_scaled.q[i].v[j]);
		}

	unordered_set<coords3d> wavefrontVerts_set(originalVerts.begin(), originalVerts.end()); // Dedupe
	vector <coords3d> refinedVerts(wavefrontVerts_set.begin(), wavefrontVerts_set.end()); // Copy to ordinary vector

	ofstream wffile;
	wffile.open(filename);
	wffile << "# This Wavefront Object file was generated by Arena.\n";
	wffile << "\n";
	wffile << "# " << refinedVerts.size() <<" Vertices.\n";
	wffile << "\n";

	for (int i = 0; i < refinedVerts.size(); i++)
		wffile << "v " << refinedVerts[i].x << " " << refinedVerts[i].y << " " << refinedVerts[i].z << "\n";
	
	// We collect the indices matching to the ARA structure first, so that we can reverse the winding to conform to the OBJ format.
	
	struct wavefrontFace
	{
		int vIndx[4]= { 0,0,0,0 };
	};

	vector <wavefrontFace> wf; // We now map refinedVerts to indices.
	
	for (int i = 0 ; i < ara_scaled.quad_count ; i++)
	{
		wavefrontFace w;

		for (int j = 0; j < 4; j++)
		{
			for (int k = 0; k < refinedVerts.size(); k++)
			{
				if (ara_scaled.q[i].v[j] == refinedVerts[k]) // There are many ajacement vertices, but we only need to find the first.
				{
					w.vIndx[j] = k + 1;
					//refinedVerts.erase(refinedVerts.begin() + k); // remove it and restart.
					break;
				}
			}
		}

		wf.push_back(w);
	}

	string p, f;
	getPathAndFilename(filename, p, f);

	wffile << "\n";
	wffile << "o "<< f << "\n";
	wffile << "\n";

	wffile << "# " << wf.size() << " Faces.\n";
	wffile << "\n";

	for (int i = 0; i < wf.size(); i++) // Now stitch faces, in reverse order.
		wffile << "f " << wf[i].vIndx[0] << "// " // We repeat the index for the normal because they are synced. NEEDS REPLACED WITH CUSTOM NORM INDICES.
			           << wf[i].vIndx[1] << "// "
			           << wf[i].vIndx[2] << "// "
			           << wf[i].vIndx[3] << "//\n";

	wffile.close();

	cout << "Export complete.\n";
}

bool obj_IsVertexLine(string line)
{
	string tmp = "";

	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] != ' ')
			tmp = tmp + line[i];
		else
			break;
	}

	if (tmp == "v")
		return true;
	else
		return false;
}

bool obj_IsTexcoordLine(string line)
{
	string tmp = "";

	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] != ' ')
			tmp = tmp + line[i];
		else
			break;
	}

	if (tmp == "vt")
		return true;
	else
		return false;
}

bool obj_IsNormalLine(string line)
{
	string tmp = "";

	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] != ' ')
			tmp = tmp + line[i];
		else
			break;
	}

	if (tmp == "vn")
		return true;
	else
		return false;
}

bool obj_IsFaceLine(string line)
{
	string tmp = "";

	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] != ' ')
			tmp = tmp + line[i];
		else
			break;
	}

	if (tmp == "f")
		return true;
	else
		return false;
}

coords3d lineTo3D(string line)
{
	coords3d c;

	string tmp = "";
	int spaceCount = 0;
	bool lastAssigned = false;

	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] == ' ' && spaceCount == 0)
		{
			tmp = "";		// Ignore description (v, vn..)
			spaceCount++;
		}
		else if (line[i] == ' ' && spaceCount == 1)
		{
			c.x = stof(tmp);
			tmp = "";
			spaceCount++;

		}
		else if (line[i] == ' ' && spaceCount == 2)
		{
			c.y = stof(tmp);
			tmp = "";
			spaceCount++;
		}
		else if (line[i] == ' ' && spaceCount == 3)
		{
			lastAssigned = true;
			c.z = stof(tmp);
			tmp = "";
			spaceCount++;
		}

		if ( line[i] != ' ')
			tmp = tmp + line[i];
	}

	if (lastAssigned == false)
		c.z = stof(tmp);

	return c;
}

point lineTo2D(string line)
{
	point c;

	string tmp = "";
	int spaceCount = 0;
	bool lastAssigned = false;

	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] == ' ' && spaceCount == 0)
		{
			tmp = "";		// Ignore description (v, vn..)
			spaceCount++;
		}
		else if (line[i] == ' ' && spaceCount == 1)
		{
			c.x = stof(tmp);
			tmp = "";
			spaceCount++;

		}
		else if (line[i] == ' ' && spaceCount == 2)
		{
			lastAssigned = true;
			c.y = stof(tmp);
			tmp = "";
			spaceCount++;
		}

		if (line[i] != ' ')
			tmp = tmp + line[i];
	}

	if (lastAssigned == false)
		c.y = stof(tmp);

	return c;
}

int FaceVertexCount(string line)
{
	// If line has 4 spaces in it "f 2/3/6 7/2/6 1/3/4 8/2/5", it's a quad.

	int spaceCount = 0;

	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] == ' ')
			spaceCount++;
	}

	return spaceCount;
}

bool lineHasSlash(string line)
{
	bool hasSlash = false;

	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] == '/')
			hasSlash = true;
	}

	return hasSlash;
}

Quad quadFromLine(string line)
{
	Quad q;

	q.tex_name = wavefront_reference;

	string tmp = "";

	int verts[4];
	int texcoords[4];
	int parseLevel = 0;
	int j = -1;

	// 1. Tokenize the verts.

	if (lineHasSlash(line) == true)
	{
		for (int i = 0; i < line.size(); i++)
		{
			//cout << "Tokenizing verts from line = " << line << "\n";
			if (line[i] == ' ')
			{
				tmp = "";
				parseLevel = 0;
				j++;
			}
			else if (line[i] != '/')
				tmp = tmp + line[i];
			else if (line[i] == '/')  // This assumes we have a slash, it's possible it doesn't.
			{
				if (parseLevel == 0) // vert 1 vert
				{
					verts[j] = atoi(tmp.c_str());
					tmp = "";
					parseLevel++;
				}
				else if (parseLevel == 1) // vert 1 texCoord
				{
					texcoords[j] = atoi(tmp.c_str());
					tmp = "";
					parseLevel++;
				}
				else if (parseLevel == 2) // vert 1 normal
					tmp = "";
			}
		}
	}
	else // For stand-alone vertex only faces.
	{
		int spaceCount = 0;

		for (int i = 0; i < line.size(); i++)
		{
			if (line[i] == ' ')
			{
				if (spaceCount > 0)
				{
					verts[spaceCount - 1] = atoi(tmp.c_str());
				}
				spaceCount++;
				tmp = "";
			}
			else
				tmp = tmp + line[i];
		}

		verts[3] = atoi(tmp.c_str());
	}

	// 2. Extract coords from indices.

	//cout << "Extracting coords from indices..\n";

	ipoint subpos;
	ipoint subdim;
	gl->gettextureDimensions(wavefront_reference, subpos, subdim);

	for (int i = 0; i < 4; i++)
	{
		q.v[i] = { vertex_list[verts[i] - 1].x * 150.0f, vertex_list[verts[i] - 1].y * 150.0f, vertex_list[verts[i] - 1].z * 150.0f }; // Scale it up a little.

		if (i == 0) // "A"
		{
			if (intex_bool == true)
			{
				q.t[i].x = texcoord_list[texcoords[i] - 1].x * (float)subdim.x;
				q.t[i].y = texcoord_list[texcoords[i] - 1].y * (float)subdim.y;
			}
			else
			{
				q.t[i].x = pinch_int;
				q.t[i].y = pinch_int;
			}
		}
		else if (i == 1) // "B"
		{
			if (intex_bool == true)
			{
				q.t[i].x = -(1.0f - texcoord_list[texcoords[i] - 1].x) * (float)subdim.x;
				q.t[i].y = texcoord_list[texcoords[i] - 1].y * (float)subdim.y;
			}
			else
			{
				q.t[i].x = -pinch_int;
				q.t[i].y = pinch_int;
			}
		}
		else if (i == 2) // "C"
		{
			if (intex_bool == true)
			{
				q.t[i].x = -(1.0f - texcoord_list[texcoords[i] - 1].x) * (float)subdim.x;
				q.t[i].y = -(1.0f - texcoord_list[texcoords[i] - 1].y) * (float)subdim.y;
			}
			else
			{
				q.t[i].x = -pinch_int;
				q.t[i].y = -pinch_int;
			}
		}
		else if (i == 3) // "D"
		{
			if (intex_bool == true)
			{
				q.t[i].x = texcoord_list[texcoords[i] - 1].x * (float)subdim.x;
				q.t[i].y = -(1.0f - texcoord_list[texcoords[i] - 1].y) * (float)subdim.y;
			}
			else
			{
				q.t[i].x = pinch_int;
				q.t[i].y = -pinch_int;
			}
		}
	}

	return q;
}

Quad triFromLine(string line) // We only use A,B,C of the quad.
{
	Quad q;

	q.tex_name = wavefront_reference;

	string tmp = "";

	int verts[4];
	int texcoords[4];
	int parseLevel = 0;
	int j = -1;

	// 1. Tokenize the verts.

	if (lineHasSlash(line) == true)
	{
		for (int i = 0; i < line.size(); i++)
		{
			if (line[i] == ' ')
			{
				tmp = "";
				parseLevel = 0;
				j++;
			}
			else if (line[i] != '/')
				tmp = tmp + line[i];
			else if (line[i] == '/')
			{
				if (parseLevel == 0) // vert 1 vert
				{
					verts[j] = atoi(tmp.c_str());
					tmp = "";
					parseLevel++;
				}
				else if (parseLevel == 1) // vert 1 texCoord
				{
					texcoords[j] = atoi(tmp.c_str());
					tmp = "";
					parseLevel++;
				}
				else if (parseLevel == 2) // vert 1 normal
					tmp = "";
			}
		}
	}
	else // For stand-alone vertex only faces.
	{
		int spaceCount = 0;

		for (int i = 0; i < line.size(); i++)
		{
			if (line[i] == ' ')
			{
				if (spaceCount > 0)
				{
					verts[spaceCount - 1] = atoi(tmp.c_str());
				}
				spaceCount++;
				tmp = "";
			}
			else
				tmp = tmp + line[i];
		}

		verts[2] = atoi(tmp.c_str());
		verts[3] = atoi(tmp.c_str());
	}

	// 2. Extract coords from indices.

	ipoint subpos;
	ipoint subdim;
	gl->gettextureDimensions(wavefront_reference, subpos, subdim);

	for (int i = 0; i < 3; i++)
	{
		q.v[i] = { vertex_list[verts[i] - 1].x * 150.0f, vertex_list[verts[i] - 1].y * 150.0f, vertex_list[verts[i] - 1].z * 150.0f }; // Scale it up a little.

		if (i == 0) // "A"
		{
			if (intex_bool == true)
			{
				q.t[i].x = texcoord_list[texcoords[i] - 1].x * (float)subdim.x;
				q.t[i].y = texcoord_list[texcoords[i] - 1].y * (float)subdim.y;
			}
			else
			{
				q.t[i].x = pinch_int;
				q.t[i].y = pinch_int;
			}
		}
		else if (i == 1) // "B"
		{
			if (intex_bool == true)
			{
				q.t[i].x = -(1.0f - texcoord_list[texcoords[i] - 1].x) * (float)subdim.x;
				q.t[i].y = texcoord_list[texcoords[i] - 1].y * (float)subdim.y;
			}
			else
			{
				q.t[i].x = -pinch_int;
				q.t[i].y = pinch_int;
			}
		}
		else if (i == 2) // "C"
		{
			if (intex_bool == true)
			{
				q.t[i].x = -(1.0f - texcoord_list[texcoords[i] - 1].x) * (float)subdim.x;
				q.t[i].y = -(1.0f - texcoord_list[texcoords[i] - 1].y) * (float)subdim.y;
			}
			else
			{
				q.t[i].x = -pinch_int;
				q.t[i].y = -pinch_int;
			}
		}
	}

	// Repeat C for D to degenerate it.

	q.v[3] = { vertex_list[verts[2] - 1].x * 150.0f, vertex_list[verts[2] - 1].y * 150.0f, vertex_list[verts[2] - 1].z * 150.0f }; // Scale it up a little.

	if (intex_bool == true)
	{
		q.t[3].x = texcoord_list[texcoords[2] - 1].x * (float)subdim.x;
		q.t[3].y = -(1.0f - texcoord_list[texcoords[2] - 1].y) * (float)subdim.y;
	}
	else
	{
		q.t[3].x = pinch_int;
		q.t[3].y = -pinch_int;
	}

	return q;
}

vector<string> tuplesFromLine(string line) // Simply breaks up each vertex tuple V/T/N into a string vector.
{
	vector<string> vt;

	string portion = "";

	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] != ' ')
			portion = portion + line[i];
		else
		{
			vt.emplace_back(portion);
			portion = "";
		}
	}

	vt.erase(vt.begin()); // Remove the "f" picked up in the first token.

	return vt;
}

coords3d vertexFromTuple(string t) // Family includes texcoordFromTuple
{
	coords3d v;

	string portion_string = "";

	for (int i = 0; i < t.size(); i++)
	{
		if (t[i] != '/')
			portion_string = portion_string + t[i];
		else
			break;
	}

	int portion_int = atoi(portion_string.c_str());
	v = vertex_list[portion_int - 1]; // Native unscaled vertex.

	return v;
}

point texcoordFromTuple(string t)
{
	point tx;

	return tx;
}

coords3d findCentralVertex(vector<string> vt)
{
	coords3d accum_vertex;

	for (int i = 0; i < vt.size(); i++)
		accum_vertex = accum_vertex + vertexFromTuple(vt[i]);

	accum_vertex.x = accum_vertex.x / vt.size();
	accum_vertex.y = accum_vertex.y / vt.size();
	accum_vertex.z = accum_vertex.z / vt.size();

	return accum_vertex;
}

vector<Quad> stitchExoticPatch(vector<coords3d> patch_list, coords3d centre_vert)
{
	vector<Quad> qv;

	Quad q;

	for (int i = 0; i < patch_list.size(); i++)
	{
		q.tex_name = wavefront_reference;
		q.v[0] = { patch_list[i].x * 150.0f, patch_list[i].y * 150.0f, patch_list[i].z * 150.0f };
		if ( i == patch_list.size()-1) // Wrap-around to first index at the end to close it.
			q.v[1] = { patch_list[0].x * 150.0f, patch_list[0].y * 150.0f, patch_list[0].z * 150.0f };
		else
			q.v[1] = { patch_list[i + 1].x * 150.0f, patch_list[i + 1].y * 150.0f, patch_list[i + 1].z * 150.0f };
		q.v[2] = { centre_vert.x * 150.0f, centre_vert.y * 150.0f, centre_vert.z * 150.0f };
		q.v[3] = { centre_vert.x * 150.0f, centre_vert.y * 150.0f, centre_vert.z * 150.0f };
		qv.emplace_back(q);
	}

	return qv;
}

vector<Quad> exoticFromLine( string line )
{
	vector<Quad> exg;

	if (lineHasSlash(line) == true)
	{
		vector<string> vt;
		vt = tuplesFromLine(line);
		coords3d cv = findCentralVertex(vt);

		vector <coords3d> vert_surface;

		for (int i = 0; i < vt.size(); i++)
			vert_surface.emplace_back(vertexFromTuple(vt[i])); // The verts we'll be stitching with.

		exg = stitchExoticPatch(vert_surface, cv);
	}
	else
	{
		cout << "!!! Vertex-Only exotic surface, not yet supported !!!\n"; // No slashes v/t/n
	}

	return exg;
}

void loading_obj_file(string filename)
{
	cout << "Loading Wavefront Object. - "<<filename<<"\n";

	static bool firstTime = true;

	vertex_list.clear();
	texcoord_list.clear();

	string line;
	ifstream objfile(filename);

	int quadCount = 0;
	int triCount = 0;
	int exoticCount = 0;

	minBound = { 0.0f, 0.0f, 0.0f }; // remember to attempt to calculate this along the way.
	maxBound = { 0.0f, 0.0f, 0.0f };

	if (objfile.is_open())
	{
		ara_original.q.clear();
		ara_original.m = Material();

		while (!objfile.eof())
		{
			getline(objfile, line);

			if (obj_IsVertexLine(line) == true)
				vertex_list.emplace_back(lineTo3D(line));
			else if (obj_IsTexcoordLine(line) == true)
				texcoord_list.emplace_back(lineTo2D(line));
			else if (obj_IsFaceLine(line) == true)
			{
				if (FaceVertexCount(line) == 4) // Is a Quad
				{
					//cout << "It's a quad!\n";
					Quad q;
					q = quadFromLine(line);
					ara_original.q.emplace_back(q);

					coords3d col = { 1.0f, 1.0f, 1.0f }; // We default vertex color to "white"
					q.quad_handle = gl->loadimage_patch(q.tex_name, q.v[0], q.v[1], q.v[2], q.v[3], col, col, col, col, q.t[0], q.t[1], q.t[2], q.t[3], q.n[0], q.n[1], q.n[2], q.n[3], ara_original.m.fresnel, ara_original.m.envmap, ara_original.m.alpha);
					gl->glassimage(q.quad_handle, 1.0f);

					// Can we create the ara_original at this point?

					quadCount++;
				}
				else if (FaceVertexCount(line) == 3) // is it a triangle..
				{
					//cout << "It's a triangle!\n";
					Quad q;
					q = triFromLine(line);
					ara_original.q.emplace_back(q);

					coords3d col = { 1.0f, 1.0f, 1.0f }; // We default vertex color to "white"
					q.quad_handle = gl->loadimage_patch(q.tex_name, q.v[0], q.v[1], q.v[2], q.v[3], col, col, col, col, q.t[0], q.t[1], q.t[2], q.t[3], q.n[0], q.n[1], q.n[2], q.n[3], ara_original.m.fresnel, ara_original.m.envmap, ara_original.m.alpha);
					gl->glassimage(q.quad_handle, 1.0f);

					triCount++;
				}
				else
				{
					vector<Quad> exoticGeo = exoticFromLine(line);

					for (int i = 0; i < exoticGeo.size(); i++)
					{
						Quad q;
						q = exoticGeo[i];
						
						ara_original.q.emplace_back(q);

						coords3d col = { 1.0f, 1.0f, 1.0f }; // We default vertex color to "white"
						q.quad_handle = gl->loadimage_patch(q.tex_name, q.v[0], q.v[1], q.v[2], q.v[3], col, col, col, col, q.t[0], q.t[1], q.t[2], q.t[3], q.n[0], q.n[1], q.n[2], q.n[3], ara_original.m.fresnel, ara_original.m.envmap, ara_original.m.alpha);
						gl->glassimage(q.quad_handle, 1.0f);
					}

					exoticCount++;
				}
			}
		}

		cout << quadCount << " quads found.\n";
		cout << triCount << " triangles found.\n";
		cout << exoticCount << " exotics found. (Averaged centre texcoord not yet calculated.)\n";

		objfile.close();
	}
	else
	{
		cout << "Unable to open Wavefront file.\n";
	}

	int total_geom = quadCount + triCount;
	ara_original.quad_count = total_geom;
	ara_transformed.quad_count = total_geom;
	ara_original.m = Material();

	string qtb = "Quads: " + to_string(ara_original.quad_count);
	quadCountbuff->text(qtb.c_str());

	// Set the widgets with the properties of this Wavefront object.

	xpos_counter->value("0.0");
	ypos_counter->value("0.0");
	zpos_counter->value("0.0");
	xscale_counter->value("1.0");
	yscale_counter->value("1.0");
	zscale_counter->value("1.0");
	xrot_counter->value("0.0");
	yrot_counter->value("0.0");
	zrot_counter->value("0.0");

	envmap_slider->value(ara_original.m.envmap);
	fresnel_slider->value(ara_original.m.fresnel);
	alpha_slider->value(ara_original.m.alpha);

	mixratio_slider->value(0.0);  	// Wavefront doesn't support these properties, so we turn them off on the UI.
	zoffset_counter->value("0.00");
	zoffset_roller->value(0.00);
	zoom_counter->value("0.00");
	imgbrowser->value(0);

	//if (ara_original.m.reverse_winding == true)
	//	winding_checkbox->value(1);
	//if (ara_original.m.reverse_winding == false)
		winding_checkbox->value(0);

	ara_original.m.orthotex_image = "";
	ara_original.m.orthotex_Zoffset = 0.0f;
	ara_original.m.orthotex_mixratio = 0.0f;
	ara_original.m.orthotex_zoom = 0.0f;

	ara_transformed.m = ara_original.m;

	coords3d dims = { maxBound.x - minBound.x, maxBound.y - minBound.y, maxBound.z - minBound.z };
	string tb = "Dimensions: " + to_trimmedString(dims.x) + ", " + to_trimmedString(dims.y) + ", " + to_trimmedString(dims.z) + ", Spatial: min(" + to_trimmedString(minBound.x) + ", " + to_trimmedString(minBound.y) + ", " + to_trimmedString(minBound.z) + ") max(" + to_trimmedString(maxBound.x) + ", " + to_trimmedString(maxBound.y) + ", " + to_trimmedString(maxBound.z) + ")";
	textbuff->text(tb.c_str());

	global_transformObject();
	Fl::redraw();
}

void loading_ara_file(string filename, bool additive, Transform t)
{
	if (additive == false)
	{
		cout << "Loading ARA specification. (" << filename << ")\n";

		minBound = { 0.0f, 0.0f, 0.0f };
		maxBound = { 0.0f, 0.0f, 0.0f };
	}
	else
		cout << "Merging ARA specification. (" << filename << ")\n";

	ifstream arafile;
	arafile.open(filename, ios::binary);

	string::size_type sz;

	arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
	string version;
	version.resize(sz);
	arafile.read(&version[0], sz);

	arafile.read(reinterpret_cast<char*>(&ara_original.quad_count), sizeof(int));
	string qtb = "Quads: " + to_string(ara_original.quad_count);
	quadCountbuff->text(qtb.c_str());

	int winding = 0;
	arafile.read(reinterpret_cast<char*>(&winding), sizeof(int)); // Winding

	if (winding == 0)
		ara_original.m.reverse_winding = false;
	else if (winding == 1)
		ara_original.m.reverse_winding = true;

	arafile.read(reinterpret_cast<char*>(&ara_original.m.fresnel), sizeof(float));
	arafile.read(reinterpret_cast<char*>(&ara_original.m.envmap), sizeof(float));
	arafile.read(reinterpret_cast<char*>(&ara_original.m.alpha), sizeof(float));

	if (version == "1.00") // Deprecated autotex and multitex
	{
		cout << "Version 1.00 - Loading deprecated features 'autotex' and 'multitex'.\n";

		string dummy_string = "";
		arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
		dummy_string.resize(sz);
		arafile.read(&dummy_string[0], sz);

		float dummy_float = 0.0f; // Load deprecated 
		arafile.read(reinterpret_cast<char*>(&dummy_float), sizeof(float));
		arafile.read(reinterpret_cast<char*>(&dummy_float), sizeof(float));
		arafile.read(reinterpret_cast<char*>(&dummy_float), sizeof(float));
		arafile.read(reinterpret_cast<char*>(&dummy_float), sizeof(float));

		arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
		dummy_string.resize(sz);
		arafile.read(&dummy_string[0], sz);

		arafile.read(reinterpret_cast<char*>(&dummy_float), sizeof(float));
		arafile.read(reinterpret_cast<char*>(&dummy_float), sizeof(float));

		ara_original.m.orthotex_image = "";
		ara_original.m.orthotex_zoom = 0.0f;
		ara_original.m.orthotex_mixratio = 0.0f;
		ara_original.m.orthotex_Zoffset = 0.0f;
	}
	else if (version == "1.01")
	{
		arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
		ara_original.m.orthotex_image.resize(sz);
		arafile.read(&ara_original.m.orthotex_image[0], sz);

		arafile.read(reinterpret_cast<char*>(&ara_original.m.orthotex_zoom), sizeof(float));
		arafile.read(reinterpret_cast<char*>(&ara_original.m.orthotex_mixratio), sizeof(float));
		arafile.read(reinterpret_cast<char*>(&ara_original.m.orthotex_Zoffset), sizeof(float));
	}

	if ( additive == false ) // Loading normally from scratch, otherwise we're trying to accumulate geometry here.
		ara_original.q.clear();

	Quad q;

	for (int i = 0; i < ara_original.quad_count; i++)
	{
		arafile.read(reinterpret_cast<char*>(&sz), sizeof(string::size_type));
		q.tex_name.resize(sz);
		arafile.read(&q.tex_name[0], sz);

		for (int j = 0; j < 4; j++)
		{
			arafile.read(reinterpret_cast<char*>(&q.v[j].x), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&q.v[j].y), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&q.v[j].z), sizeof(float));
		}

		for (int j = 0; j < 4; j++)
		{
			arafile.read(reinterpret_cast<char*>(&q.t[j].x), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&q.t[j].y), sizeof(float));
		}

		for (int j = 0; j < 4; j++)
		{
			arafile.read(reinterpret_cast<char*>(&q.n[j].x), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&q.n[j].y), sizeof(float));
			arafile.read(reinterpret_cast<char*>(&q.n[j].z), sizeof(float));
		}

		for (int j = 0; j < 4; j++)
		{
			// Determine minimum boundary.
			if (q.v[j].x < minBound.x)
				minBound.x = q.v[j].x;

			if (q.v[j].y < minBound.y)
				minBound.y = q.v[j].y;

			if (q.v[j].z < minBound.z)
				minBound.z = q.v[j].z;

			// Determine maximum boundary.
			if (q.v[j].x > maxBound.x)
				maxBound.x = q.v[j].x;

			if (q.v[j].y > maxBound.y)
				maxBound.y = q.v[j].y;

			if (q.v[j].z > maxBound.z)
				maxBound.z = q.v[j].z;
		}

		if (additive == true) // If we're accumulating ARAs, we just ignore the global ARA orthotex setting, because that's gonna look weird.
		{
			ara_original.m.orthotex_image = "";
			ara_original.m.orthotex_zoom = 0.0f;
			ara_original.m.orthotex_mixratio = 0.0f;
			ara_original.m.orthotex_Zoffset = 0.0f;
		}

		coords3d col = { 1.0f, 1.0f, 1.0f }; // We default vertex color to "white"

		if (additive == true)
		{
			q.v[0] = gl->transform(q.v[0], { -t.rotate.x, -t.rotate.y, -t.rotate.z });
			q.v[1] = gl->transform(q.v[1], { -t.rotate.x, -t.rotate.y, -t.rotate.z });
			q.v[2] = gl->transform(q.v[2], { -t.rotate.x, -t.rotate.y, -t.rotate.z });
			q.v[3] = gl->transform(q.v[3], { -t.rotate.x, -t.rotate.y, -t.rotate.z });

			q.v[0] = q.v[0] * t.scale.x;
			q.v[1] = q.v[1] * t.scale.x;
			q.v[2] = q.v[2] * t.scale.x;
			q.v[3] = q.v[3] * t.scale.x;

			q.v[0] = q.v[0] + t.translate;
			q.v[1] = q.v[1] + t.translate;
			q.v[2] = q.v[2] + t.translate;
			q.v[3] = q.v[3] + t.translate;
		}

		if (ara_original.m.reverse_winding == false)
			q.quad_handle = gl->loadimage_patch(q.tex_name, q.v[0], q.v[1], q.v[2], q.v[3], col, col, col, col, q.t[0], q.t[1], q.t[2], q.t[3], q.n[0], q.n[1], q.n[2], q.n[3], ara_original.m.fresnel, ara_original.m.envmap, ara_original.m.alpha);

		else if (ara_original.m.reverse_winding == true)
			q.quad_handle = gl->loadimage_patch_r(q.tex_name, q.v[0], q.v[1], q.v[2], q.v[3], col, col, col, col, q.t[0], q.t[1], q.t[2], q.t[3], q.n[0], q.n[1], q.n[2], q.n[3], ara_original.m.fresnel, ara_original.m.envmap, ara_original.m.alpha);

		if (ara_original.m.orthotex_image != "")
			gl->orthoteximage(q.quad_handle, ara_original.m.orthotex_image, ara_original.m.orthotex_zoom, ara_original.m.orthotex_mixratio, ara_original.m.orthotex_Zoffset);

		gl->glassimage(q.quad_handle, 1.0);

		ara_original.q.emplace_back(q);
	}

	arafile.close();

	// Set the widgets with the properties of this ARA object.

	xpos_counter->value("0.00");
	ypos_counter->value("0.00");
	zpos_counter->value("0.00");
	xscale_counter->value("1.00");
	yscale_counter->value("1.00");
	zscale_counter->value("1.00");
	xrot_counter->value("0.00");
	yrot_counter->value("0.00");
	zrot_counter->value("0.00");

	envmap_slider->value(ara_original.m.envmap);
	fresnel_slider->value(ara_original.m.fresnel);
	alpha_slider->value(ara_original.m.alpha);

	mixratio_slider->value(ara_original.m.orthotex_mixratio);
	zoffset_counter->value(to_trimmedString(ara_original.m.orthotex_Zoffset).c_str());
	zoffset_roller->value(0.00);
	zoom_counter->value(to_trimmedString(ara_original.m.orthotex_zoom).c_str());
	
	if (ara_original.m.orthotex_image != "")
	{
		int i = 1;
		while (imgbrowser->text(i) != NULL)
		{
			string imageline = imgbrowser->text(i);
			if (imageline == ara_original.m.orthotex_image)
				break;
			i++;
		}

		imgbrowser->display(i);
		imgbrowser->value(i);
	}
	else
	{
		imgbrowser->display(1);
		imgbrowser->value(1);
	}

	if (ara_original.m.reverse_winding == true)
		winding_checkbox->value(1);
	if (ara_original.m.reverse_winding == false)
		winding_checkbox->value(0);

	coords3d dims = { maxBound.x - minBound.x, maxBound.y - minBound.y, maxBound.z - minBound.z };
	string tb = "Dimensions: " + to_trimmedString(dims.x) + ", " + to_trimmedString(dims.y) + ", " + to_trimmedString(dims.z) + ", Spatial: min(" + to_trimmedString(minBound.x) + ", " + to_trimmedString(minBound.y) + ", " + to_trimmedString(minBound.z) + ") max(" + to_trimmedString(maxBound.x) + ", " + to_trimmedString(maxBound.y) + ", " + to_trimmedString(maxBound.z) + ")";
	textbuff->text(tb.c_str());

	if (additive == false) // Only refresh the view if this is a single file.
	{
		global_transformObject();
		Fl::redraw();
	}
}

void saving_scn_as_ara_file(string filename)
{
	cout << "Saving SCN to ARA specification.\n";

	ofstream SCNfile(filename, ios::out | ios::binary);
	if (!SCNfile)
	{
		cout << "ARENA cannot for some reason save the ARA file.\n";
		return;
	}

	if (S.size() > 0)
	{
		EmptyObject.index_type = 1; // Clear Main
		EmptyObject.index = 0;
		EmptyObject.isStatic = true;
		gl->resetimagesto(EmptyObject);

		ara_original.q.clear();
		global_transformObject();

		int totalQuads = 0;

		for (int i = 0; i < S.size(); i++)
		{
			Transform t;
			t.rotate = S[i].rotate;
			t.scale.x = S[i].scale;
			t.translate = S[i].translate;

			//cout << "translate = " << t.translate.x << ", " << t.translate.y << ", " << t.translate.z << "\n";
			//cout << "scale = " << t.scale.x << "\n";
			//cout << "rotate = " << t.rotate.x << ", " << t.rotate.y << ", " << t.rotate.z << "\n";

			string totalfile = S[i].object_path + S[i].object_file;

			loading_ara_file(totalfile, true, t);
			totalQuads = totalQuads + ara_original.quad_count;
		}

		ara_original.quad_count = totalQuads;
		saving_ara_file(filename);

		EmptyObject.index_type = 1; // Clear Main
		EmptyObject.index = 0;
		EmptyObject.isStatic = true;
		gl->resetimagesto(EmptyObject);

		ara_original.q.clear();
		global_transformObject();

		if (initial_ara != "")
		{
			Transform ot;
			loading_ara_file(initial_ara, false, ot);
		}

		EmptyObject.index_type = 1; // Clear Main, to avoid the Object drawing with the Scene.
		EmptyObject.index = 0;
		EmptyObject.isStatic = true;
		gl->resetimagesto(EmptyObject);

		scnAfterLoadHash = getSceneHash(); // "AfterLoad" is a bit of a misnomer in this case, but it's required to align them.
		scnCurrentHash = scnAfterLoadHash;
	}
	else
		cout << "Scene is empty, nothing to save as ARA.\n";
}

class arena : public Fl_Double_Window
{
private:

	static void menu_save_scn(Fl_Widget*, void*)
	{
		file_chooser = new Fl_Native_File_Chooser;
		file_chooser->title("Save SCN");
		file_chooser->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
		file_chooser->filter("SCN\t*.scn");
		file_chooser->show();

		if (file_chooser->filename() != "")
		{
			cout << "filename = " << file_chooser->filename() << "\n";
			saving_scn_file(file_chooser->filename());
		}
		else
			cout << "Cancelled from dialog.\n";
	}

	static void menu_save_scn_as_ara(Fl_Widget*, void*)
	{
		file_chooser = new Fl_Native_File_Chooser;
		file_chooser->title("Save SCN to ARA");
		file_chooser->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
		file_chooser->filter("ARA\t*.ara");
		file_chooser->show();

		if (file_chooser->filename() != "")
		{
			cout << "filename = " << file_chooser->filename() << "\n";
			saving_scn_as_ara_file(file_chooser->filename());
		}
		else
			cout << "Cancelled from dialog.\n";
	}

	static void menu_save_ara(Fl_Widget*, void*)
	{
		file_chooser = new Fl_Native_File_Chooser;
		file_chooser->title("Save ARA");
		file_chooser->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
		file_chooser->filter("ARA\t*.ara");
		file_chooser->show();

		if (file_chooser->filename() != "")
		{
			cout << "filename = " << file_chooser->filename() << "\n";
			static marker_type EmptyObject;
			EmptyObject.index_type = 1; // Main
			EmptyObject.index = 0;
			EmptyObject.isStatic = true;
			gl->resetimagesto(EmptyObject);
			saving_ara_file(file_chooser->filename());
		}
		else
			cout << "Cancelled from dialog.\n";
	}

	static void menu_new_scn(Fl_Widget*, void*)
	{
		if (scnCurrentHash != 0 && scnCurrentHash != scnAfterLoadHash)
			wantToSave_cb(mtabs1, NULL);

		EmptyObject.index_type = 1; // Clear Main
		EmptyObject.index = 0;
		EmptyObject.isStatic = true;
		gl->resetimagesto(EmptyObject);

		EmptyObject.index_type = 2; // Clear ZC
		gl->resetZCimagesto(EmptyObject);

		scene_browser->clear();

		S.clear();
		mtabs1->value(scn_group);

		scn_posx_counter->value("0.00");
		scn_posy_counter->value("0.00");
		scn_posz_counter->value("0.00");
		scn_scale_counter->value("1.00");
		scn_rotx_counter->value("0.00");
		scn_roty_counter->value("0.00");
		scn_rotz_counter->value("0.00");

		obj_mode = false;

		Fl::redraw();
	}

	static void menu_import_wavefront(Fl_Widget*, void*)
	{
		file_chooser = new Fl_Native_File_Chooser;
		file_chooser->title("Import Wavefront Object");
		file_chooser->type(Fl_Native_File_Chooser::BROWSE_MULTI_FILE);
		file_chooser->filter("OBJ\t*.obj");
		file_chooser->show();

		if (file_chooser->filename() != "")
		{
			static marker_type EmptyObject;
			EmptyObject.index_type = 1; // Main
			EmptyObject.index = 0;
			EmptyObject.isStatic = true;
			gl->resetimagesto(EmptyObject);

			obj_mode = true;

			EmptyObject.index_type = 2; // Clear ZC, Flush the scene anyway.
			gl->resetZCimagesto(EmptyObject);
			mtabs1->value(obj_group); // Switch to Object tab.
			Fl::redraw();

			loading_obj_file(file_chooser->filename());
		}
		else
			cout << "Cancelled from dialog.\n";
	}

	static void menu_export_wavefront(Fl_Widget*, void*)
	{
		file_chooser = new Fl_Native_File_Chooser;
		file_chooser->title("Export to Wavefront");
		file_chooser->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
		file_chooser->filter("OBJ\t*.obj");
		file_chooser->show();

		if (file_chooser->filename() != "")
		{
			cout << "filename = " << file_chooser->filename() << "\n";
			static marker_type EmptyObject;
			EmptyObject.index_type = 1; // Main
			EmptyObject.index = 0;
			EmptyObject.isStatic = true;
			gl->resetimagesto(EmptyObject);
			exporting_obj_file(file_chooser->filename());
		}
		else
			cout << "Cancelled from dialog.\n";

	}

	static void wantToSave_cb(Fl_Widget* widget, void* data)
	{
		int choice = fl_choice("Do you want to save this Scene?", "Yes", "No", "Still No");
		if (choice == 0)
		{
			// User clicked "Yes"
			menu_save_scn(mtabs1, NULL);
		}
		else if (choice == 1)
		{
			// User clicked "No", carry on to load a new SCN file.
		}
	}

	static void menu_load_scn(Fl_Widget*, void*)
	{
		if (scnCurrentHash != 0 && scnCurrentHash != scnAfterLoadHash)
			wantToSave_cb(mtabs1, NULL);

		file_chooser = new Fl_Native_File_Chooser;
		file_chooser->title("Load SCN");
		file_chooser->type(Fl_Native_File_Chooser::BROWSE_MULTI_FILE);
		file_chooser->filter("SCN\t*.scn");
		file_chooser->show();

		if (file_chooser->filename() != "")
		{
			cout << "filename = " << file_chooser->filename() << "\n";
			static marker_type EmptyObject;
			EmptyObject.index_type = 1; // Main
			EmptyObject.index = 0;
			EmptyObject.isStatic = true;
			gl->resetimagesto(EmptyObject);

			obj_mode = false;

			EmptyObject.index_type = 2; // Clear ZC, Flush the scene anyway.
			gl->resetZCimagesto(EmptyObject);
			S.clear();
			mtabs1->value(scn_group); // Set the Object tab for this.
			gl->pushVBO();
			gl->pushShaders();
			Fl::redraw();

			loading_scn_file(file_chooser->filename());
		}
		else
			cout << "Cancelled from dialog.\n";
	}

	static void menu_load_ara(Fl_Widget*, void*)
	{
		file_chooser = new Fl_Native_File_Chooser;
		file_chooser->title("Load ARA");
		file_chooser->type(Fl_Native_File_Chooser::BROWSE_MULTI_FILE);
		file_chooser->filter("ARA\t*.ara");
		file_chooser->show();

		if (file_chooser->filename() != "")
		{
			static marker_type EmptyObject;
			EmptyObject.index_type = 1; // Main
			EmptyObject.index = 0;
			EmptyObject.isStatic = true;

			obj_mode = true;

			gl->resetimagesto(EmptyObject);

			EmptyObject.index_type = 2; // Clear ZC, Flush the scene anyway.
			gl->resetZCimagesto(EmptyObject);
			mtabs1->value(obj_group); // Set the Object tab for this.
			gl->pushVBO();
			gl->pushShaders();
			Fl::redraw();

			Transform t;
			initial_ara = file_chooser->filename(); // Save the existing ARA.
			loading_ara_file(file_chooser->filename(), false, t);
		}
		else
			cout << "Cancelled from dialog.\n";
	}

	static void menu_load_ice(Fl_Widget*, void*)
	{
		file_chooser = new Fl_Native_File_Chooser;
		file_chooser->title("Load ICE");
		file_chooser->type(Fl_Native_File_Chooser::BROWSE_MULTI_FILE);
		file_chooser->filter("ICE\t*.ice");
		file_chooser->show();

		if (file_chooser->filename() != "")
		{
			static marker_type EmptyObject;
			EmptyObject.index_type = 1; // Main
			EmptyObject.index = 0;
			EmptyObject.isStatic = true;
			gl->resetimagesto(EmptyObject);

			obj_mode = true;

			EmptyObject.index_type = 2; // Clear ZC, Flush the scene anyway.
			gl->resetZCimagesto(EmptyObject);
			mtabs1->value(obj_group); // Switch to Object tab.
			Fl::redraw();

			loading_ice_file(file_chooser->filename());
		}
		else
			cout << "Cancelled from dialog.\n";
	}

	static void menu_quit(Fl_Widget*, void*)
	{
		gl->shutdown();
	}

	static void dummy(Fl_Widget*, void*)
	{
		cout << "Implement this.\n";
	}

	static void switchTab(Fl_Widget*, void*)
	{
		if (mtabs1->value() == obj_group)
		{
			obj_mode = true;

			static marker_type EmptyObject;
			EmptyObject.index_type = 2; // ZC
			EmptyObject.index = 0;
			EmptyObject.isStatic = true;
			gl->resetZCimagesto(EmptyObject);

			//lastNormalsCalced = 0; // Make it regenerate ara_transformed's Normals.
			transformObject(mtabs1, (void*)true);

			string qtb = "Quads: " + to_string(ara_original.quad_count);
			quadCountbuff->text(qtb.c_str());

			Fl::redraw();
		}
		else if (mtabs1->value() == scn_group)
		{
			//int b = gl->loadHUDobject("assets\/menu_button.ara");
			//gl->moveHUDobject(b, { 100,100, 0 });
			//gl->scaleHUDobject(b, 0.95);
			//gl->glassHUDobject(b, 1.0f);

			obj_mode = false;

			static marker_type EmptyObject;
			EmptyObject.index_type = 1; // Main
			EmptyObject.index = 0;
			EmptyObject.isStatic = true;
			gl->resetimagesto(EmptyObject);

			transformScene(mtabs1, NULL);

			marker_type sceneCount = gl->getMarker_ZC();
			string qtb = "Scene: " + to_string(sceneCount.index);
			quadCountbuff->text(qtb.c_str());

			Fl::redraw();
		}
	}

	static void camera_toggle(Fl_Widget*, void*)
	{
		if (cameraOn == true)
		{
			cameraOn = false;
			ShowCursor(true);
		}
		else if (cameraOn == false)
		{
			cameraOn = true;
			ShowCursor(false);
			SetCursorPos(640, 410);
		}

		//Fl::redraw();
	}

	static void transformObject(Fl_Widget* w, void* p)
	{
		bool recalc_norm = (bool*) p;
		if (recalc_norm == true)
			lastNormalsCalced = 0;

		if (ara_original.quad_count > 0)
		{
			EmptyObject.index_type = 1; // Clear Main
			EmptyObject.index = 0;
			EmptyObject.isStatic = true;
			gl->resetimagesto(EmptyObject); // Clear the object to re-build.

			Transform t;
			if (xpos_counter->size() > 0 && ypos_counter->size() > 0 && zpos_counter->size() > 0)
				t.translate = { stof(xpos_counter->value()), stof(ypos_counter->value()), stof(zpos_counter->value()) };
			if (xrot_counter->size() > 0 && yrot_counter->size() > 0 && zrot_counter->size() > 0)
				t.rotate = { stof(xrot_counter->value()), stof(yrot_counter->value()), stof(zrot_counter->value()) };
			if (xscale_counter->size() > 0 && yscale_counter->size() > 0 && zscale_counter->size() > 0)
				t.scale = { stof(xscale_counter->value()), stof(yscale_counter->value()), stof(zscale_counter->value()) };

			Material m;
			m.alpha = alpha_slider->value();
			m.envmap = envmap_slider->value();
			m.fresnel = fresnel_slider->value();

			if (light_facet_button->value() == 1)
			{
				m.faceted = true;
				ara_transformed.m.faceted = true;
			}
			else if (light_average_button->value() == 1)
			{
				m.faceted = false;
				ara_transformed.m.faceted = false;
			}

			if (winding_checkbox->value() == 1)
				m.reverse_winding = true;
			else
				m.reverse_winding = false;

			if (imgbrowser->value() > 0) // Something was selected.
			{
				m.orthotex_image = imgbrowser->text(imgbrowser->value());

				if (zoffset_roller->changed() != 0) // If roller changed value, over-write zoffset counter.
				{
					m.orthotex_Zoffset = zoffset_roller->value();
					zoffset_counter->value(to_trimmedString(zoffset_roller->value()).c_str());
				}
				else
					m.orthotex_Zoffset = atoi(zoffset_counter->value());

				m.orthotex_zoom = stof(zoom_counter->value());
				m.orthotex_mixratio = mixratio_slider->value();
			}

			ara_transformed.m = m;
			ara_transformed.quad_count = ara_original.quad_count;

			coords3d colr = { 1.0f, 1.0f, 1.0f };

			Object ara_previous = ara_transformed; // Attempt to hang on to pre-calculated normals we don't store elsewhere.
			ara_transformed.q.clear();

			Quad q;

			current_textures.clear();
			replacement_browser->clear();

			coords3d mds = gl->mouseDepth(); // collect this for the quad tex paint.
			int mds_closest_i = -1; // Find the closest Quad to the mouse projection point.
			float mds_closest_vertex = 100000.0f;

			for (int i = 0 ; i < ara_original.quad_count ; i++)
			{
				q.tex_name = ara_original.q[i].tex_name;
				current_textures.insert(q.tex_name); // For the texture replacement function
				q.complete = 0;

				// First translate
				q.v[0] = ara_original.q[i].v[0] + t.translate;
				q.v[1] = ara_original.q[i].v[1] + t.translate;
				q.v[2] = ara_original.q[i].v[2] + t.translate;
				q.v[3] = ara_original.q[i].v[3] + t.translate;
				
				// Rotate
				q.v[0] = gl->transform(q.v[0], t.rotate);
				q.v[1] = gl->transform(q.v[1], t.rotate);
				q.v[2] = gl->transform(q.v[2], t.rotate);
				q.v[3] = gl->transform(q.v[3], t.rotate);

				// Scale
				q.v[0] = q.v[0] * t.scale;
				q.v[1] = q.v[1] * t.scale;
				q.v[2] = q.v[2] * t.scale;
				q.v[3] = q.v[3] * t.scale;

				q.t[0] = ara_original.q[i].t[0];
				q.t[1] = ara_original.q[i].t[1];
				q.t[2] = ara_original.q[i].t[2];
				q.t[3] = ara_original.q[i].t[3];

				if (quadToggle_checkbox->value() == 1) // Quad Texture Paint
				{
					coords3d avg = { (q.v[0].x + q.v[1].x + q.v[2].x + q.v[3].x) / 4.0f,
									 (q.v[0].y + q.v[1].y + q.v[2].y + q.v[3].y) / 4.0f,
									 (q.v[0].z + q.v[1].z + q.v[2].z + q.v[3].z) / 4.0f };

					float dist = gl->DistanceBetween3D(avg, mds);

					if (dist < mds_closest_vertex)
					{
						mds_closest_vertex = dist;
						mds_closest_i = i;
					}
				}

				ara_transformed.q.emplace_back(q);
			}

			if (quadToggle_checkbox->value() == 1 && quadBrowser->value() > 0) // Quad Texture Paint
			{
				ara_transformed.q[mds_closest_i].tex_name = quadBrowser->text(quadBrowser->value()); // Temporary because it's only applied to the final Transformation.

				if (quadRotate_slider->value() == 0)
				{
					point dims = gl->getimageDimensions(ara_original.q[mds_closest_i].quad_handle); // We fetch from "original" because apparently we never copied to "transform".
					int pinch = quadPinch_spinner->value();

					ara_transformed.q[mds_closest_i].t[0].x = pinch;
					ara_transformed.q[mds_closest_i].t[0].y = pinch;

					ara_transformed.q[mds_closest_i].t[1].x = -pinch;
					ara_transformed.q[mds_closest_i].t[1].y = pinch;

					ara_transformed.q[mds_closest_i].t[2].x = -pinch;
					ara_transformed.q[mds_closest_i].t[2].y = -pinch;

					ara_transformed.q[mds_closest_i].t[3].x = pinch;
					ara_transformed.q[mds_closest_i].t[3].y = -pinch;
				}
				
				else if (quadRotate_slider->value() == 90)
				{
					point dims = gl->getimageDimensions(ara_original.q[mds_closest_i].quad_handle); // We fetch from "original" because apparently we never copied to "transform".
					int pinch = quadPinch_spinner->value();

					ara_transformed.q[mds_closest_i].t[0].x = dims.x - pinch;
					ara_transformed.q[mds_closest_i].t[0].y = pinch;

					ara_transformed.q[mds_closest_i].t[1].x = -pinch;
					ara_transformed.q[mds_closest_i].t[1].y = dims.y - pinch;

					ara_transformed.q[mds_closest_i].t[2].x = -dims.x + pinch;
					ara_transformed.q[mds_closest_i].t[2].y = -pinch;

					ara_transformed.q[mds_closest_i].t[3].x = pinch;
					ara_transformed.q[mds_closest_i].t[3].y = -dims.y + pinch;
				}

				else if (quadRotate_slider->value() == 180)
				{
					point dims = gl->getimageDimensions(ara_original.q[mds_closest_i].quad_handle); // We fetch from "original" because apparently we never copied to "transform".
					int pinch = quadPinch_spinner->value();

					ara_transformed.q[mds_closest_i].t[0].x = dims.x - pinch;
					ara_transformed.q[mds_closest_i].t[0].y = dims.y - pinch;

					ara_transformed.q[mds_closest_i].t[1].x = -dims.x + pinch;
					ara_transformed.q[mds_closest_i].t[1].y = dims.y - pinch;

					ara_transformed.q[mds_closest_i].t[2].x = -dims.x + pinch;
					ara_transformed.q[mds_closest_i].t[2].y = -dims.y + pinch;

					ara_transformed.q[mds_closest_i].t[3].x = dims.x - pinch;
					ara_transformed.q[mds_closest_i].t[3].y = -dims.y + pinch;
				}

				else if (quadRotate_slider->value() == 270)
				{
					point dims = gl->getimageDimensions(ara_original.q[mds_closest_i].quad_handle); // We fetch from "original" because apparently we never copied to "transform".
					int pinch = quadPinch_spinner->value();

					ara_transformed.q[mds_closest_i].t[0].x = pinch;
					ara_transformed.q[mds_closest_i].t[0].y = dims.y - pinch;

					ara_transformed.q[mds_closest_i].t[1].x = -dims.x + pinch;
					ara_transformed.q[mds_closest_i].t[1].y = pinch;

					ara_transformed.q[mds_closest_i].t[2].x = -pinch;
					ara_transformed.q[mds_closest_i].t[2].y = -dims.y + pinch;

					ara_transformed.q[mds_closest_i].t[3].x = dims.x - pinch;
					ara_transformed.q[mds_closest_i].t[3].y = -pinch;
				}

				bool mclick = gl->mouse_click(); // Maybe in future we stick this in the Timer, because this expires as you use it.
				if (mclick == true) // Apply the texture.
				{
					ara_original.q[mds_closest_i].tex_name = quadBrowser->text(quadBrowser->value());
					ara_original.q[mds_closest_i].t[0] = ara_transformed.q[mds_closest_i].t[0];
					ara_original.q[mds_closest_i].t[1] = ara_transformed.q[mds_closest_i].t[1];
					ara_original.q[mds_closest_i].t[2] = ara_transformed.q[mds_closest_i].t[2];
					ara_original.q[mds_closest_i].t[3] = ara_transformed.q[mds_closest_i].t[3];
				}
			}

			current_browser->clear();
			for (auto i : current_textures)
				current_browser->add(i.c_str());

			if (ara_transformed.m.faceted == false && lastNormalsCalced != 1) // ..either 0 or 2
			{
				lastNormalsCalced = 1; // Average mode, so we don't need to calc again, if already done.
				calc_quad_normals();
				generate_average_normals();
			}
			else if (ara_transformed.m.faceted == true && lastNormalsCalced != 2) // ..either 0 or 1
			{
				lastNormalsCalced = 2; // Faceted.
				calc_quad_normals();
			}
			else
			{
				// Pass the normals as they are.

				for (int i = 0; i < ara_original.quad_count; i++)
					for (int j = 0; j < 4; j++)
						ara_transformed.q[i].n[j] = ara_previous.q[i].n[j];
			}

			minBound = { 0.0f, 0.0f, 0.0f };
			maxBound = { 0.0f, 0.0f, 0.0f };

			gl->suppressMissingImages(true);

			for (int i = 0; i < ara_original.quad_count; i++) // Draw the primatives.
			{
				for (int j = 0; j < 4; j++)
				{
					// Determine minimum boundary.
					if (ara_transformed.q[i].v[j].x < minBound.x)
						minBound.x = ara_transformed.q[i].v[j].x;

					if (ara_transformed.q[i].v[j].y < minBound.y)
						minBound.y = ara_transformed.q[i].v[j].y;

					if (ara_transformed.q[i].v[j].z < minBound.z)
						minBound.z = ara_transformed.q[i].v[j].z;

					// Determine maximum boundary.
					if (ara_transformed.q[i].v[j].x > maxBound.x)
						maxBound.x = ara_transformed.q[i].v[j].x;

					if (ara_transformed.q[i].v[j].y > maxBound.y)
						maxBound.y = ara_transformed.q[i].v[j].y;

					if (ara_transformed.q[i].v[j].z > maxBound.z)
						maxBound.z = ara_transformed.q[i].v[j].z;
				}

				int handle = -1;

				if (maintexOffset_checkbox->value() == 1 ) // Use the over-ride texcoords in Materials dialog instead.
				{
					ipoint ta = {1,1}; // Default values, in case the counter doesn't have a valid value.
					ipoint tb = {-1,1};
					ipoint tc = {-1,-1};
					ipoint td = {1,-1};

					if (maintexOffset_counter->size() > 0)
					{
						int tv = atoi(maintexOffset_counter->value());
						ta = { tv, tv };
						tb = { -tv, tv };
						tc = { -tv, -tv };
						td = { tv, -tv };
					}

					if (m.reverse_winding == false)
						handle = gl->loadimage_patch(ara_transformed.q[i].tex_name, ara_transformed.q[i].v[0], ara_transformed.q[i].v[1], ara_transformed.q[i].v[2], ara_transformed.q[i].v[3], colr, colr, colr, colr, ta, tb, tc, td, ara_transformed.q[i].n[0], ara_transformed.q[i].n[1], ara_transformed.q[i].n[2], ara_transformed.q[i].n[3], ara_transformed.m.fresnel, ara_transformed.m.envmap, ara_transformed.m.alpha);
					else if (m.reverse_winding == true)
						handle = gl->loadimage_patch_r(ara_transformed.q[i].tex_name, ara_transformed.q[i].v[0], ara_transformed.q[i].v[1], ara_transformed.q[i].v[2], ara_transformed.q[i].v[3], colr, colr, colr, colr, ta, tb, tc, td, ara_transformed.q[i].n[0], ara_transformed.q[i].n[1], ara_transformed.q[i].n[2], ara_transformed.q[i].n[3], ara_transformed.m.fresnel, ara_transformed.m.envmap, ara_transformed.m.alpha);
				}
				else
				{
					if (m.reverse_winding == false)
						handle = gl->loadimage_patch(ara_transformed.q[i].tex_name, ara_transformed.q[i].v[0], ara_transformed.q[i].v[1], ara_transformed.q[i].v[2], ara_transformed.q[i].v[3], colr, colr, colr, colr, ara_transformed.q[i].t[0], ara_transformed.q[i].t[1], ara_transformed.q[i].t[2], ara_transformed.q[i].t[3], ara_transformed.q[i].n[0], ara_transformed.q[i].n[1], ara_transformed.q[i].n[2], ara_transformed.q[i].n[3], ara_transformed.m.fresnel, ara_transformed.m.envmap, ara_transformed.m.alpha);
					else if (m.reverse_winding == true)
						handle = gl->loadimage_patch_r(ara_transformed.q[i].tex_name, ara_transformed.q[i].v[0], ara_transformed.q[i].v[1], ara_transformed.q[i].v[2], ara_transformed.q[i].v[3], colr, colr, colr, colr, ara_transformed.q[i].t[0], ara_transformed.q[i].t[1], ara_transformed.q[i].t[2], ara_transformed.q[i].t[3], ara_transformed.q[i].n[0], ara_transformed.q[i].n[1], ara_transformed.q[i].n[2], ara_transformed.q[i].n[3], ara_transformed.m.fresnel, ara_transformed.m.envmap, ara_transformed.m.alpha);
				}

				gl->glassimage(handle, 1.0f);

				if (m.orthotex_image != "")
					gl->orthoteximage(handle, m.orthotex_image, m.orthotex_zoom, m.orthotex_mixratio, m.orthotex_Zoffset);
			}

			gl->suppressMissingImages(false);

			if (originx_checkbox->value() == 1)
			{
				int xchecker = gl->loadobject("assets\/checker.ara"); // TODO don't keep -loading this!
				if (xchecker != -1)
				{
					gl->rotateobject(xchecker, { 90.0f, 90.0f, 0.0f });
					gl->glassobject(xchecker, (float)origin_slider->value());
				}
			}

			if (originy_checkbox->value() == 1)
			{
				int ychecker = gl->loadobject("assets\/checker.ara");
				if (ychecker != -1)
				{
					gl->rotateobject(ychecker, { 0.0f, 90.0f, 0.0f });
					gl->glassobject(ychecker, (float)origin_slider->value());
				}
			}

			if (originz_checkbox->value() == 1)
			{
				int zchecker = gl->loadobject("assets\/checker.ara");
				if (zchecker != -1)
				{
					gl->rotateobject(zchecker, { 0.0f, 0.0f, 0.0f });
					gl->glassobject(zchecker, (float)origin_slider->value());
				}
			}

			coords3d dims = { maxBound.x - minBound.x, maxBound.y - minBound.y, maxBound.z - minBound.z };
			string tb = "Dimensions: " + to_trimmedString(dims.x) + ", " + to_trimmedString(dims.y) + ", " + to_trimmedString(dims.z) + ", Spatial: min(" + to_trimmedString(minBound.x) + ", " + to_trimmedString(minBound.y) + ", " + to_trimmedString(minBound.z) + ") max(" + to_trimmedString(maxBound.x) + ", " + to_trimmedString(maxBound.y) + ", " + to_trimmedString(maxBound.z) + ")";
			textbuff->text(tb.c_str());

			Fl::redraw();
		}
	}

	static void transformScene(Fl_Widget*, void*)
	{
		if (S.size() > 0)
		{
			int selectedObject = scene_browser->value();
			if (selectedObject > 0)
			{
				if (scn_posx_counter->size() > 0 && scn_posy_counter->size() > 0 && scn_posz_counter->size() > 0)
					S[selectedObject - 1].translate = { stof(scn_posx_counter->value()), stof(scn_posy_counter->value()), stof(scn_posz_counter->value()) };
				if (scn_rotx_counter->size() > 0 && scn_roty_counter->size() > 0 && scn_rotz_counter->size() > 0)
					S[selectedObject - 1].rotate = { stof(scn_rotx_counter->value()), stof(scn_roty_counter->value()), stof(scn_rotz_counter->value()) };
				if (scn_scale_counter->size() > 0)
					S[selectedObject - 1].scale = stof(scn_scale_counter->value());
			}

			EmptyObject.index_type = 2; // Clear ZC
			gl->resetZCimagesto(EmptyObject);

			gl->suppressMissingImages(true);

			scnGlobalPos = { 0.0f, 0.0f, 0.0f };
			if (scn_gblx_counter->size() > 0 && scn_gbly_counter->size() > 0 && scn_gblz_counter->size() > 0) // Global Position
			{
				scnGlobalPos.x = stof(scn_gblx_counter->value());
				scnGlobalPos.y = stof(scn_gbly_counter->value());
				scnGlobalPos.z = stof(scn_gblz_counter->value());
			}

			for (int i = 0; i < S.size(); i++)
			{
				int o = gl->loadZCobject(S[i].object_path + S[i].object_file);
				gl->moveZCobject(o, { S[i].translate.x + scnGlobalPos.x, S[i].translate.y + scnGlobalPos.y, S[i].translate.z + scnGlobalPos.z });
				gl->rotateZCobject(o, S[i].rotate);
				gl->scaleZCobject(o, S[i].scale);
				gl->glassZCobject(o, 1.0f);
			}

			gl->suppressMissingImages(false);

			Fl::redraw();
		}

		scnCurrentHash = getSceneHash();
	}

	static void centreObject(Fl_Widget*, void*)
	{
		if (ara_original.quad_count > 0)
		{
			coords3d avg_centre = { 0.0f, 0.0f, 0.0f };

			for (int i = 0; i < ara_original.quad_count; i++)
			{
				avg_centre = avg_centre + ara_original.q[i].v[0];
				avg_centre = avg_centre + ara_original.q[i].v[1];
				avg_centre = avg_centre + ara_original.q[i].v[2];
				avg_centre = avg_centre + ara_original.q[i].v[3];
			}

			coords3d avg_total = { avg_centre.x / (ara_original.quad_count * 4.0f), avg_centre.y / (ara_original.quad_count * 4.0f) , avg_centre.z / (ara_original.quad_count * 4.0f) };
			string avg;
			avg = to_trimmedString(-avg_total.x);
			xpos_counter->value(avg.c_str());
			avg = to_trimmedString(-avg_total.y);
			ypos_counter->value(avg.c_str());
			avg = to_trimmedString(-avg_total.z);
			zpos_counter->value(avg.c_str());
		}
	}

	static void cameraDelta(Fl_Widget*cc, void*)
	{
		if (((Fl_Float_Input*)cc)->size() > 0)
			cameraSpeed = stof(((Fl_Float_Input*)cc)->value());
		cout << "Adjusting camera speed\n";
	}

	static void transferdownTex_cb(Fl_Widget*, void*)
	{
		if (current_browser->value() > 0)
			replacement_browser->add(current_browser->text(current_browser->value()));
	}

	static void transferupTex_cb(Fl_Widget*, void*)
	{
		if (atlas_browser->value() > 0)
			replacement_browser->add(atlas_browser->text(atlas_browser->value()));
	}

	static void clearTex_cb(Fl_Widget*, void*)
	{
		replacement_browser->clear();
	}

	static void replaceTex_cb(Fl_Widget*, void*)
	{
		if (replacement_browser->size() > 0 && (current_browser->size() == replacement_browser->size()))
		{
			for (int i = 0; i < replacement_browser->size(); i++) // Either browser size would have been fine.
			{
				for (int j = 0; j < ara_original.q.size(); j++)
				{
					if (current_browser->text(i + 1) == ara_original.q[j].tex_name)
						ara_original.q[j].tex_name = replacement_browser->text(i + 1);
				}
			}
		}

		transformObject(mtabs1, (void*)true);
	}

	static void object_browse_cb(Fl_Widget*, void*)
	{
		file_chooser = new Fl_Native_File_Chooser;
		file_chooser->title("Add Object to Scene");
		file_chooser->type(Fl_Native_File_Chooser::BROWSE_FILE);
		file_chooser->filter("ARA\t*.ara");
		file_chooser->show();

		if (file_chooser->filename() != "")
		{
			string fn = file_chooser->filename();
			string ffp; // Path
			string ffn; // Name

			getPathAndFilename(fn, ffp, ffn);

			scene_browser->add(ffn.c_str());

			Scene s;

			s.object_file = ffn;
			s.object_path = ffp;
			s.translate = { 0.0f, 0.0f, 0.0f };
			s.rotate = { 0.0f, 0.0f, 0.0f };
			s.scale = 1.0f;

			S.emplace_back(s);

			gl->suppressMissingImages(true);
			int o = gl->loadZCobject(fn);
			gl->suppressMissingImages(false);
			gl->moveZCobject(o, { 0.0f, 0.0f, 0.0f });
			gl->glassZCobject(o, 1.0f);

			marker_type sceneCount = gl->getMarker_ZC();
			string qtb = "Scene: " + to_string(sceneCount.index);
			quadCountbuff->text(qtb.c_str());

			Fl::redraw();
		}
		else
			cout << "Cancelled from dialog.\n";
	}

	static void scene_browser_cb(Fl_Widget*, void*)
	{
		if (scene_browser->value() > 0)
		{
			scn_posx_counter->value(to_trimmedString(S[scene_browser->value() - 1].translate.x).c_str());
			scn_posy_counter->value(to_trimmedString(S[scene_browser->value() - 1].translate.y).c_str());
			scn_posz_counter->value(to_trimmedString(S[scene_browser->value() - 1].translate.z).c_str());
			scn_rotx_counter->value(to_trimmedString(S[scene_browser->value() - 1].rotate.x).c_str());
			scn_roty_counter->value(to_trimmedString(S[scene_browser->value() - 1].rotate.y).c_str());
			scn_rotz_counter->value(to_trimmedString(S[scene_browser->value() - 1].rotate.z).c_str());
			scn_scale_counter->value(to_trimmedString(S[scene_browser->value() - 1].scale).c_str());
		}
	}

	static void object_delete_cb(Fl_Widget*, void*)
	{
		int selectedObject = scene_browser->value();
		if (selectedObject > 0)
		{
			scene_browser->remove(selectedObject);
			S.erase(S.begin() + (selectedObject - 1));
			transformScene(mtabs1, NULL);

			marker_type sceneCount = gl->getMarker_ZC();
			string qtb = "Scene: " + to_string(sceneCount.index);
			quadCountbuff->text(qtb.c_str());
		}
	}

	static void setWavefrontTexture_cb(Fl_Widget*, void*)
	{
		if (wavefront_browser->value() > 0) // Something was selected.
			wavefront_reference = wavefront_browser->text(wavefront_browser->value());

		cout << "wavefront reference = " << wavefront_reference << "\n";
	}

	static void ortho_cb(Fl_Widget*, void*)
	{
		if (ortho_checkbox->value() == 1)
		{
			cout << "Setting camera to Ortho mode.\n";
			gl->setOrthoMode(true);
		}
		else if (ortho_checkbox->value() == 0)
		{
			cout << "Setting camera to Perspective mode.\n";
			gl->setOrthoMode(false);
		}

		Fl::redraw();
	}

	static void backface_cb(Fl_Widget*, void*)
	{
		if (backface_checkbox->value() == 1)
		{
			cout << "See backfaces.\n";
			gl->backfacecull(false);
		}
		else if (backface_checkbox->value() == 0)
		{
			cout << "Single-side face mode (normal).\n";
			gl->backfacecull(true);
		}

		Fl::redraw();
	}

	static void wireframe_cb(Fl_Widget*, void*)
	{
		if (wireframe_checkbox->value() == 1)
		{
			cout << "Wireframe on.\n";
			gl->wireframe(true);
		}
		else if (wireframe_checkbox->value() == 0)
		{
			cout << "Wireframe off.\n";
			gl->wireframe(false);
		}

		Fl::redraw();
	}

	static void origin_cb(Fl_Widget*, void*)
	{
		//cout << "Origin used.\n";

		transformObject(mtabs1, (void*)false);

		Fl::redraw();
	}

	static void intex_cb(Fl_Widget*, void*)
	{
		if (intex_checkbox->value() == 1)
			intex_bool = true;
		else if (intex_checkbox->value() == 0)
			intex_bool = false;
	}

	static void pinch_cb(Fl_Widget*, void*)
	{
		if (pinch_counter->size() > 0)
			pinch_int = atoi(pinch_counter->value());
	}

	static void scnReorder_cb(Fl_Widget*, void* p)
	{
		int p_int = (int)p;

		if (p_int == 1)
		{
			if (S.size() > 0 && scene_browser->value() > 1 ) // > 1 (index 0) because it can't be more up than 1.
			{
				swap(S[scene_browser->value()-1], S[scene_browser->value() - 2]);
				int row = scene_browser->value() - 1;
	
				scene_browser->clear();
				for (int i = 0; i < S.size(); i++)
					scene_browser->add(S[i].object_file.c_str());

				scene_browser->value(row);
				transformScene(mtabs1, NULL);
			}
		}
		else if (p_int == 2)
		{
			if (S.size() > 0 && scene_browser->value() < S.size())
			{
				swap(S[scene_browser->value() - 1], S[scene_browser->value()]);
				int row = scene_browser->value() + 1;

				scene_browser->clear();
				for (int i = 0; i < S.size(); i++)
					scene_browser->add(S[i].object_file.c_str());

				scene_browser->value(row);
				transformScene(mtabs1, NULL);
			}
		}
	}

	static void HUDquad( string image, point pos, point dims, coords3d color)
	{
		//coords3d col = { 1.0f, 1.0f, 1.0f }; // We default vertex color to "white"

		ipoint ta = { 5,5 }; // Default values, in case the counter doesn't have a valid value.
		ipoint tb = { -5,5 };
		ipoint tc = { -5,-5 };
		ipoint td = { 5,-5 };

		int handle = gl->loadHUDimage_patch(image, { pos.x, pos.y, 1.0f}, { pos.x + dims.x, pos.y, 1.0f }, { pos.x + dims.x, pos.y + dims.y, 1.0f }, { pos.x, pos.y + dims.y, 1.0f }, color, color, color, color, ta, tb, tc, td, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, 6.0f, 0.0f, 1.0f);
		gl->glassHUDimage(handle, 1.0f);
	}

	static void rail_cb(Fl_Widget*, void* p)
	{
		float railwidth_float = 1.0f;
		float railgauge_float = 1.0f;
		int railsteps_int = 1;

		if (railwidth_counter->size() > 0)
			railwidth_float = stof(railwidth_counter->value());

		if (railgauge_counter->size() > 0)
			railgauge_float = stof(railgauge_counter->value());

		if (railsteps_counter->size() > 0)
			railsteps_int = atoi(railsteps_counter->value());

		if (rail_browser->value() > 0) // If a rail image was selected, go ahead.
		{
			float angleDelta = 90.0f / float(railsteps_int);
			float angle = 90.0f; // Start angle

			struct edge
			{
				coords3d a = { 0.0f, 0.0f, 0.0f };
				coords3d b = { 0.0f, 0.0f, 0.0f };
			};

			vector <edge> leftRail;
			vector <edge> rightRail;

			edge le; // left edge
			edge re; // right edge

			le.a = { 0.0f, (32.0f - railgauge_float) - railwidth_float, 0.0f };
			le.b = { 0.0f, (32.0f - railgauge_float) + railwidth_float, 0.0f };
			re.a = { 0.0f, (32.0f + railgauge_float) - railwidth_float, 0.0f };
			re.b = { 0.0f, (32.0f + railgauge_float) + railwidth_float, 0.0f };

			point p0 = gl->cosine(le.a.y, angle);
			point p1 = gl->cosine(le.b.y, angle);
			edge le_temp = { {p0.x, p0.y, 0.0f}, {p1.x, p1.y, 0.0f} };

			point p2 = gl->cosine(re.a.y, angle);
			point p3 = gl->cosine(re.b.y, angle);
			edge re_temp = { {p2.x, p2.y, 0.0f}, {p3.x, p3.y, 0.0f} };

			leftRail.push_back(le_temp); // Initial position.
			rightRail.push_back(re_temp);

			angle = angle + angleDelta;

			for (int i = 0; i < railsteps_int; i++) // Lay out the edges.
			{
				point p0 = gl->cosine(le.a.y, angle);
				point p1 = gl->cosine(le.b.y, angle);
				edge le_temp = { {p0.x, p0.y, 0.0f}, {p1.x, p1.y, 0.0f} };

				point p2 = gl->cosine(re.a.y, angle);
				point p3 = gl->cosine(re.b.y, angle);
				edge re_temp = { {p2.x, p2.y, 0.0f}, {p3.x, p3.y, 0.0f} };

				leftRail.push_back(le_temp);
				rightRail.push_back(re_temp);

				angle = angle + angleDelta;
			}

			coords3d col = { 1.0f, 1.0f, 1.0f }; // We default vertex color to "white"

			ipoint ta = { 1,1 }; // Default values, in case the counter doesn't have a valid value.
			ipoint tb = { -1,1 };
			ipoint tc = { -1,-1 };
			ipoint td = { 1,-1 };

			marker_type hud_mt;
			hud_mt.index = 0;
			hud_mt.index_type = 3;
			hud_mt.isStatic = true;
			gl->resetHUDimagesto(hud_mt);

			HUDquad("white0", { 0.0f,0.0f }, { 64.0f,64.0f }, {1,1,1});
			HUDquad("white0", { 64.0f,64.0f }, { 64.0f,64.0f }, { 1,1,1 });

			for (int i = 0; i < railsteps_int; i++) // Stitch the patches.
			{
				int handle = gl->loadHUDimage_patch(rail_browser->text(rail_browser->value()), leftRail[i].a, leftRail[i].b, leftRail[i + 1].b, leftRail[i + 1].a, col, col, col, col, ta, tb, tc, td, {0.0f, 0.0f, -1.0f}, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, 6.0f, 0.0f, 1.0f);
				gl->glassHUDimage(handle, 1.0f);
				    handle = gl->loadHUDimage_patch(rail_browser->text(rail_browser->value()), rightRail[i].a, rightRail[i].b, rightRail[i + 1].b, rightRail[i + 1].a, col, col, col, col, ta, tb, tc, td, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, 6.0f, 0.0f, 1.0f);
				gl->glassHUDimage(handle, 1.0f);

				if (i == railsteps_int - 1)
					break;
			}

			Fl::redraw();
		}
	}

	static void railsave_cb(Fl_Widget*, void* p)
	{
		cout << "Saving rail parameters.\n";
	}

	static void vertex_reset_cb(Fl_Widget*, void* p)
	{
		ifstream vfile("assets\/arena_vertex.txt");
		string vfile_contents;
		string vline;
		while (getline(vfile, vline))
			vfile_contents += vline + '\n';
		vfile.close();

		vertexbuff->text(vfile_contents.c_str());
	}

	static void vertex_compile_cb(Fl_Widget*, void* p)
	{
		const char* v_text = vertexbuff->text();
		const char* f_text = fragmentbuff->text();
		gl->reloadShaders( v_text, f_text );
		Fl::redraw();
	}

	static void vertex_save_cb(Fl_Widget*, void* p)
	{
		ofstream file("assets\/arena_vertex.txt");
		if (file.is_open())
		{
			file << vertexbuff->text();
			file.close();
		}
	}

	static void fragment_reset_cb(Fl_Widget*, void* p)
	{
		ifstream ffile("assets\/arena_fragment.txt");
		string ffile_contents;
		string fline;
		while (getline(ffile, fline))
			ffile_contents += fline + '\n';
		ffile.close();

		fragmentbuff->text(ffile_contents.c_str());
	}

	static void fragment_compile_cb(Fl_Widget*, void* p)
	{
		const char* v_text = vertexbuff->text();
		const char* f_text = fragmentbuff->text();
		gl->reloadShaders(v_text, f_text);
		Fl::redraw();
	}

	static void fragment_save_cb(Fl_Widget*, void* p)
	{
		ofstream file("assets\/arena_fragment.txt");
		if (file.is_open())
		{
			file << fragmentbuff->text();
			file.close();
		}
	}

	static void cubemap_browser_cb(Fl_Widget*, void*)
	{
		if (cubemap_browser->value() > 0)
		{
			cubemapInput->value(cubemap_browser->text(cubemap_browser->value()));

			for (int i = 0; i < cubemapData.size(); i++)
			{
				if (cubemapData[i].name == cubemap_browser->text(cubemap_browser->value()))
				{
					gl->setupenvmap(cubemapData[i].path);
					Fl::redraw();
					Fl::check();
				}
			}
		}
	}

	static void cubemap_cb(Fl_Widget*, void* p)
	{
		string input = cubemapInput->value();
		bool found = false;

		if (input != "") // Don't do anything with an empty input!
		{
			CubemapPair cmp;

			// Find if the Input is a Browser entry.
			for (int i = 0; i < cubemapData.size(); i++)
			{
				if (cubemapData[i].name == input)
				{
					// Found it in the Browser, copy it from there.
					cmp = cubemapData[i];

					found = true;
				}
			}

			// Create a new cubemap folder by lowercasing and removing the spaces.
			if (found == false)
			{
				cmp.name = input;
				input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
				std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) { return std::tolower(c); });
				cout << "lowercase and without spaces = " << input << "\n";
				cmp.path = input;
				cubemapData.push_back(cmp);
				cubemap_browser->add(cmp.name.c_str());

				// Now create the folder

				filesystem::path dir("assets/cubemaps/" + cmp.path);
				if (!filesystem::exists(dir))
					filesystem::create_directory(dir);

				// Update cubemaps_index.txt

				ofstream updatefile;
				updatefile.open("assets/cubemaps/cubemap_index.txt");
				for (int i = 0; i < cubemapData.size(); i++)
				{
					updatefile << cubemapData[i].name << "\n";
					updatefile << cubemapData[i].path << "\n";
				}
				updatefile.close();
			}

			// Get the cameras position.

			cameraOn = false; // Stop the camera from being moved around.

			persp = 90.0f;

			coords3d _cam;
			coords3d _look;
			coords3d _ZCcam;
			coords3d _ZClook;
			gl->getCameras(_cam, _look, _ZCcam, _ZClook);

			coords3d lookCM = _ZCcam;
			lookCM = { lookCM.x, lookCM.y - 10.0f, lookCM.z };
			cameraUP = { 0.0f, 0.0f, 1.0f };
			gl->cameras(_cam, _look, _ZCcam, lookCM);
			gl->takeCubemap("assets/cubemaps/" + cmp.path + "/env_p_y.dat");
			Fl::redraw();
			Fl::check();

			lookCM = _ZCcam;
			lookCM = { lookCM.x, lookCM.y + 10.0f, lookCM.z };
			gl->cameras(_cam, _look, _ZCcam, lookCM);
			cameraUP = { 0.0f, 0.0f, -1.0f };
			gl->takeCubemap("assets/cubemaps/" + cmp.path + "/env_n_y.dat");
			Fl::redraw();
			Fl::check();

			lookCM = _ZCcam;
			lookCM = { lookCM.x - 10.0f, lookCM.y + 0.0001f, lookCM.z + 0.0001f };
			cameraUP = { -1.0f, -1.0f, 0.0f };
			gl->cameras(_cam, _look, _ZCcam, lookCM);
			gl->takeCubemap("assets/cubemaps/" + cmp.path + "/env_p_x.dat");
			Fl::redraw();
			Fl::check();

			lookCM = _ZCcam;
			lookCM = { lookCM.x + 10.0f, lookCM.y, lookCM.z };
			cameraUP = { -1.0f, -1.0f, 0.0f };
			gl->cameras(_cam, _look, _ZCcam, lookCM);
			gl->takeCubemap("assets/cubemaps/" + cmp.path + "/env_n_x.dat");
			Fl::redraw();
			Fl::check();

			lookCM = _ZCcam;
			lookCM = { lookCM.x, lookCM.y + 0.0001f, lookCM.z - 10.0f};
			cameraUP = { 0.0f, 0.0f, -1.0f };
			gl->cameras(_cam, _look, _ZCcam, lookCM);
			gl->takeCubemap("assets/cubemaps/" + cmp.path + "/env_p_z.dat");
			Fl::redraw();
			Fl::check();

			lookCM = _ZCcam;
			lookCM = { lookCM.x, lookCM.y - 0.0001f, lookCM.z + 10.0f};
			gl->cameras(_cam, _look, _ZCcam, lookCM);
			gl->takeCubemap("assets/cubemaps/" + cmp.path + "/env_n_z.dat");
			Fl::redraw();
			Fl::check();

			gl->setupenvmap(cmp.name); // Load this cubemap we just made.
			persp = 45.0f; // Return to regular perspective.

			gl->cameras(_cam, _look, _ZCcam, _ZClook); // Restore the camera.
			Fl::redraw();
			Fl::check();
		}
	}

	static void delete_cubemap_cb(Fl_Widget*, void* p)
	{
		if (cubemap_browser->value() > 0) // Check the browser is pointing at something.
		{
			for (int i = 0; i < cubemapData.size(); i++)
			{
				if (cubemapData[i].name == cubemap_browser->text(cubemap_browser->value()))
				{
					// Delete from file system.
					filesystem::remove_all("assets/cubemaps/" + cubemapData[i].path);

					// Erase from vector.
					cubemapData.erase(cubemapData.begin() + i);

					// Update index file.
					ofstream updatefile;
					updatefile.open("assets/cubemaps/cubemap_index.txt");
					for (int i = 0; i < cubemapData.size(); i++)
					{
						updatefile << cubemapData[i].name << "\n";
						updatefile << cubemapData[i].path << "\n";
					}
					updatefile.close();

					// Remove from browser
					cubemap_browser->remove(i+1);

					break;
				}
			}
		}
	}

	static void importARAs(Fl_Widget*, void* p)
	{
		cout << "Importing!\n";

		string source_dir = "C:\\Users\\cammy\\source\\repos\\von Myses Railway Set\\von Myses Railway Set\\assets\\ARAversion1\\";
		string target_dir = "C:\\Users\\cammy\\source\\repos\\von Myses Railway Set\\von Myses Railway Set\\assets\\";

		vector <string> import_list;
		
		import_list.push_back("arch.ara");
		import_list.push_back("book.ara");
		import_list.push_back("budget_button.ara");
		import_list.push_back("building_button.ara");
		import_list.push_back("bulb.ara");
		import_list.push_back("cable.ara");
		import_list.push_back("carpet.ara");
		import_list.push_back("ceiling.ara");
		import_list.push_back("chair.ara");
		import_list.push_back("cupboard.ara");
		import_list.push_back("cupboard_better.ara");
		import_list.push_back("demolish_button.ara");
		import_list.push_back("door.ara");
		import_list.push_back("engine_car_button.ara");
		import_list.push_back("engine_train.ara");
		import_list.push_back("factory_building_button.ara");
		import_list.push_back("factory_car_button.ara");
		import_list.push_back("fireplace.ara");
		import_list.push_back("fireset.ara");
		import_list.push_back("floor.ara");
		import_list.push_back("food_building_button.ara");
		import_list.push_back("food_car_button.ara");
		import_list.push_back("food_train.ara");
		import_list.push_back("goods_train.ara");
		import_list.push_back("house.ara");
		import_list.push_back("housec0h0a.ara");
		import_list.push_back("housec0h0i.ara");
		import_list.push_back("housec0h1a.ara");
		import_list.push_back("housec0h1i.ara");
		import_list.push_back("housec0h2a.ara");
		import_list.push_back("housec0h2i.ara");
		import_list.push_back("housec0h3a.ara");
		import_list.push_back("housec0h3i.ara");
		import_list.push_back("housec1h0a.ara");
		import_list.push_back("housec1h0i.ara");
		import_list.push_back("housec1h1a.ara");
		import_list.push_back("housec1h1i.ara");
		import_list.push_back("housec1h2a.ara");
		import_list.push_back("housec1h2i.ara");
		import_list.push_back("housec1h3a.ara");
		import_list.push_back("housec1h3i.ara");
		import_list.push_back("housec2h0a.ara");
		import_list.push_back("housec2h0i.ara");
		import_list.push_back("housec2h1a.ara");
		import_list.push_back("housec2h1i.ara");
		import_list.push_back("housec2h2a.ara");
		import_list.push_back("housec2h2i.ara");
		import_list.push_back("housec2h3a.ara");
		import_list.push_back("housec2h3i.ara");
		import_list.push_back("lampshade.ara");
		import_list.push_back("landscape1.ara");
		import_list.push_back("lumberyard_building_button.ara");
		import_list.push_back("lumber_car_button.ara");
		import_list.push_back("mantle.ara");
		import_list.push_back("metal_building_button.ara");
		import_list.push_back("metal_car_button.ara");
		import_list.push_back("mouse_blocked.ara");
		import_list.push_back("mouse_normal.ara");
		import_list.push_back("mouse_switch.ara");
		import_list.push_back("oil_building_button.ara");
		import_list.push_back("oil_car_button.ara");
		import_list.push_back("oil_train.ara");
		import_list.push_back("painting.ara");
		import_list.push_back("passenger_car_button.ara");
		import_list.push_back("passenger_train.ara");
		import_list.push_back("pillars_original.ara");
		import_list.push_back("railscape1.ara");
		import_list.push_back("rail_button.ara");
		import_list.push_back("shadow.ara");
		import_list.push_back("shutters.ara");
		import_list.push_back("sleeper.ara");
		import_list.push_back("smoke0.ara");
		import_list.push_back("smoke1.ara");
		import_list.push_back("smoke2.ara");
		import_list.push_back("survey_button.ara");
		import_list.push_back("table.ara");
		import_list.push_back("table_leg.ara");
		import_list.push_back("table_surface.ara");
		import_list.push_back("textDisplay.ara");
		import_list.push_back("top_panel.ara");
		import_list.push_back("townhall_l1.ara");
		import_list.push_back("townpeg.ara");
		import_list.push_back("train_button.ara");
		import_list.push_back("tree_type0.ara");
		import_list.push_back("tree_type1.ara");
		import_list.push_back("tree_type2.ara");
		import_list.push_back("tree_type3.ara");
		import_list.push_back("UIwindow.ara");
		import_list.push_back("wall.ara");
		import_list.push_back("water_building_button.ara");
		import_list.push_back("water_car_button.ara");
		import_list.push_back("water_train.ara");
		import_list.push_back("window.ara");
		import_list.push_back("window_frame.ara");
		import_list.push_back("window_wall.ara");

		Transform t;

		for (int i = 0; i < import_list.size(); i++)
		{
			loading_ara_file(source_dir + import_list[i], false, t);
			saving_ara_file(target_dir + import_list[i]);
		}
	}

	static void timer_cb(void*)
	{
		static float yaw = 270.0f;
		static float pitch = 45.0f;
		static coords3d cameraFront = { 0.1f, -20, 0.1f };
		static coords3d cameraUp = { 0, 0, -1 };

		static float ZCyaw = 270.0f;
		static float ZCpitch = 45.0f;
		static coords3d ZCcameraFront = { 0.1f, -20, 0.1f };
		static coords3d ZCcameraUp = { 0, 0, -1 };

		static bool  resetCamera = true;

		ipoint mxy_screen = gl->xymouse_screen();
		ipoint mxy = gl->xymouse();
		int k = gl->getKey();

		//cout << "k = " << k << "\n";

		if (quadToggle_checkbox->value() == 1 && cameraOn == false) // Run the Quad Texture apply in real-time, but not with the camera on.
			transformObject(mtabs1, (void*)true);

		if (cameraOn == true)
		{
			int k = gl->getKey();

			if (obj_mode == true)
			{
				if (mxy_screen.x != 640 || mxy_screen.y != 410)
				{
					yaw = yaw - ((640.0f - mxy_screen.x) * 0.10); // 640.0f
					pitch = pitch - ((410.0f - mxy_screen.y) * 0.10); //410.0f

					if (pitch > 89.0f)
						pitch = 89.0f;
					if (pitch < -89.0f)
						pitch = -89.0f;

					yaw = fmod(yaw, 360.0f);
					if (yaw < 0.0f)
						yaw = 360.0f + yaw;

					SetCursorPos(640, 410);
				}

				coords3d direction;
				direction.x = cos(yaw * 0.017453);
				direction.y = sin(yaw * 0.017453) * cos(pitch * 0.017453);
				direction.z = sin(pitch * 0.017453);

				if (k == 119) // forward
					cameraPos = cameraPos + (cameraFront * cameraSpeed);
				if (k == 115) // backward
					cameraPos = cameraPos - (cameraFront * cameraSpeed);
				if (k == 97) // left
					cameraPos = cameraPos - (gl->normcrossprod(cameraFront, cameraUp) * (cameraSpeed * 2.0f));
				if (k == 100) // right
					cameraPos = cameraPos + (gl->normcrossprod(cameraFront, cameraUp) * (cameraSpeed * 2.0f));
				if (k == 114) // up
					cameraPos = cameraPos + (cameraUp * cameraSpeed);
				if (k == 102) // down
					cameraPos = cameraPos - (cameraUp * cameraSpeed);

				cameraFront = gl->normalize(direction);

				lookPos = cameraPos + cameraFront;

				string camOrient = "Camera Orientation: " + to_trimmedString(int(gl->AngleOfLine({ cameraPos.x, cameraPos.y }, { lookPos.x, lookPos.y }))) + " degrees.";
				camerabuff->text(camOrient.c_str());

				if (k == 99) // 'C' escapes camera mode
				{
					cameraOn = false;
					ShowCursor(true);
					cout << "Escaping camera.\n";
				}
			}
			else if (obj_mode == false)
			{
				if (mxy_screen.x != 640 || mxy_screen.y != 410)
				{
					ZCyaw = ZCyaw - ((640.0f - mxy_screen.x) * 0.10);
					ZCpitch = ZCpitch - ((410.0f - mxy_screen.y) * 0.10);

					if (ZCpitch > 89.0f)
						ZCpitch = 89.0f;
					if (ZCpitch < -89.0f)
						ZCpitch = -89.0f;

					ZCyaw = fmod(ZCyaw, 360.0f);
					if (ZCyaw < 0.0f)
						ZCyaw = 360.0f + ZCyaw;

					SetCursorPos(640, 410);
				}

				coords3d direction;
				direction.x = cos(ZCyaw * 0.017453);
				direction.y = sin(ZCyaw * 0.017453) * cos(ZCpitch * 0.017453);
				direction.z = sin(ZCpitch * 0.017453);

				if (k == 119) // forward
					ZCcameraPos = ZCcameraPos + (ZCcameraFront * cameraSpeed);
				if (k == 115) // backward
					ZCcameraPos = ZCcameraPos - (ZCcameraFront * cameraSpeed);
				if (k == 97) // left
					ZCcameraPos = ZCcameraPos - (gl->normcrossprod(ZCcameraFront, ZCcameraUp) * (cameraSpeed * 2.0f));
				if (k == 100) // right
					ZCcameraPos = ZCcameraPos + (gl->normcrossprod(ZCcameraFront, ZCcameraUp) * (cameraSpeed * 2.0f));
				if (k == 114) // up
					ZCcameraPos = ZCcameraPos + (ZCcameraUp * cameraSpeed);
				if (k == 102) // down
					ZCcameraPos = ZCcameraPos - (ZCcameraUp * cameraSpeed);

				ZCcameraFront = gl->normalize(direction);

				ZClookPos = ZCcameraPos + ZCcameraFront;

				string camOrient = "Camera Orientation: " + to_trimmedString(int(gl->AngleOfLine({ ZCcameraPos.x, ZCcameraPos.y }, { ZClookPos.x, ZClookPos.y }))) + " degrees.";
				camerabuff->text(camOrient.c_str());

				if (k == 99) // 'C' escapes camera mode
				{
					cameraOn = false;
					ShowCursor(true);
					cout << "Escaping ZCcamera.\n";
				}
			}

			gl->cameras(cameraPos, lookPos, ZCcameraPos, ZClookPos);
			Fl::redraw();
		}
		else if (cameraOn == false)
			resetCamera = true;

		Fl::repeat_timeout(0.05, timer_cb);
	}

public:

	arena(int W, int H, const char* L = 0) : Fl_Double_Window(W, H, L)
	{
		Fl::scheme("gtk+");

		Fl::set_color(FL_BACKGROUND_COLOR, 50, 50, 50);
		Fl::set_color(FL_BACKGROUND2_COLOR, 120, 120, 120);
		Fl::set_color(FL_FOREGROUND_COLOR, 240, 240, 240);
		Fl::reload_scheme();
		Fl::set_color(FL_SELECTION_COLOR, 200, 200, 200);

		// OpenGL window
		gl = new bluebush(5, 75, 1022, 574);

		Fl_PNG_Image* icon = new Fl_PNG_Image("assets/icon.png");
		gl->default_icon(icon);

		EmptyObject = gl->getMarker_main();		
		EmptyScene = gl->getMarker_ZC();

		menu = new Fl_Menu_Bar(0, 0, 1480, 25);
		
		menu->add("Arena/Load ARA Arena", FL_CTRL + 'a', menu_load_ara);
		menu->add("Arena/Save ARA Arena", FL_CTRL + 'c', menu_save_ara);
		
		menu->add("Scene/New SCN Scene", FL_CTRL + 'n', menu_new_scn);
		menu->add("Scene/Load SCN Scene", FL_CTRL + 'l', menu_load_scn);
		menu->add("Scene/Save SCN Scene", FL_CTRL + 's', menu_save_scn);
		menu->add("Scene/Save SCN to ARA", FL_CTRL + 'p', menu_save_scn_as_ara);
		
		menu->add("Import/Load ICE Iceberg", FL_CTRL + 'i', menu_load_ice);
		menu->add("Import/Import Wavefront", FL_CTRL + 'w', menu_import_wavefront); // Attempt to import Wavefront. Add "Default Tex" to change white0.
		menu->add("Export/Export to Wavefront", FL_CTRL + 'e', menu_export_wavefront);

		textbuff = new Fl_Text_Buffer();
		bounds_display = new Fl_Text_Display(10, 650, 1480, 25);
		bounds_display->color(FL_GRAY);
		bounds_display->buffer(textbuff);
		textbuff->text("Dimensions: Spatial:");

		camerabuff = new Fl_Text_Buffer();
		camera_display = new Fl_Text_Display(10, 675, 1480, 25);
		camera_display->color(FL_GRAY);
		camera_display->buffer(camerabuff);
		camerabuff->text("Camera Orientation:");

		Fl_PNG_Image* camera_icon = new Fl_PNG_Image("assets\/camera_icon.png");
		camera_button = new Fl_Button(0, 25, 48, 48);
		camera_button->image(camera_icon);
		camera_button->callback(camera_toggle);

		obj_mode = true;
		cameraPos = { 0.0f, 200.0f, -200.0f };
		lookPos = { 0.0f, 0.0f, 0.0f };
		ZCcameraPos = { 0.0f, 200.0f, -200.0f };
		ZClookPos = { 0.0f, 0.0f, 0.0f };

		mtabs1 = new Fl_Tabs(50, 25, 1430, 600); // was 25 down, We hide the Scene/Object tabs and move between them programmatically.
		{
			obj_group = new Fl_Group(50, 50, 1430, 575, "Object");
			{
				tabs = new Fl_Tabs(50, 50, 1430, 600);
				{
					Fl_Group* aaa = new Fl_Group(50, 75, 1430, 575, "Transform");
					{
						Fl_Group* group1 = new Fl_Group(1075, 100, 225, 128, "Translate");
						group1->box(FL_UP_FRAME);
						group1->labeltype(FL_SHADOW_LABEL);
						group1->end();

						xpos_counter = new Fl_Float_Input(1150, 120, 90, 25, "X");
						xpos_counter->callback(transformObject, (void*)false);
						xpos_counter->color(FL_GRAY);
						xpos_counter->value("0.00");
						ypos_counter = new Fl_Float_Input(1150, 150, 90, 25, "Y");
						ypos_counter->callback(transformObject, (void*)false);
						ypos_counter->color(FL_GRAY);
						ypos_counter->value("0.00");
						zpos_counter = new Fl_Float_Input(1150, 180, 90, 25, "Z");
						zpos_counter->callback(transformObject, (void*)false);
						zpos_counter->color(FL_GRAY);
						zpos_counter->value("0.00");

						Fl_Group* group2 = new Fl_Group(1075, 250, 225, 128, "Rotate");
						group2->box(FL_UP_FRAME);
						group2->labeltype(FL_SHADOW_LABEL);
						group2->end();

						xrot_counter = new Fl_Float_Input(1150, 270, 90, 25, "Xrot");
						xrot_counter->callback(transformObject, (void*)true);
						xrot_counter->color(FL_GRAY);
						xrot_counter->value("0.00");
						yrot_counter = new Fl_Float_Input(1150, 300, 90, 25, "Yrot");
						yrot_counter->callback(transformObject, (void*)true);
						yrot_counter->color(FL_GRAY);
						yrot_counter->value("0.00");
						zrot_counter = new Fl_Float_Input(1150, 330, 90, 25, "Zrot");
						zrot_counter->callback(transformObject, (void*)true);
						zrot_counter->color(FL_GRAY);
						zrot_counter->value("0.00");

						Fl_Group* group3 = new Fl_Group(1075, 400, 225, 128, "Scale");
						group3->box(FL_UP_FRAME);
						group3->labeltype(FL_SHADOW_LABEL);
						group3->end();

						xscale_counter = new Fl_Float_Input(1150, 420, 90, 25, "Xscale");
						xscale_counter->callback(transformObject, (void*)true);
						xscale_counter->color(FL_GRAY);
						xscale_counter->value("1.00");
						yscale_counter = new Fl_Float_Input(1150, 450, 90, 25, "Yscale");
						yscale_counter->callback(transformObject, (void*)true);
						yscale_counter->color(FL_GRAY);
						yscale_counter->value("1.00");
						zscale_counter = new Fl_Float_Input(1150, 480, 90, 25, "Zscale");
						zscale_counter->callback(transformObject, (void*)true);
						zscale_counter->color(FL_GRAY);
						zscale_counter->value("1.00");

						Fl_Button* centre_button = new Fl_Button(1140, 550, 100, 25, "Centre Object");
						centre_button->color(88 + 1);
						centre_button->callback(centreObject);

						Fl_Button* import_button = new Fl_Button(1140, 580, 100, 25, "Import ARAs");
						import_button->color(78 + 1);
						import_button->callback(importARAs);
					}
					aaa->end();

					Fl_Group* qta = new Fl_Group(50, 75, 1430, 575, "Quad Texture");
					{
						Fl_Group* quadtex_group;
						quadtex_group = new Fl_Group(1055, 100, 400, 545, "Paint Quad");
						quadtex_group->box(FL_UP_FRAME);
						quadtex_group->labeltype(FL_SHADOW_LABEL);
						quadtex_group->end();

						fl_register_images();

						quadBrowser = new Fl_Hold_Browser(1070, 120, 370, 400);
						quadBrowser->callback(transformObject, (void*)false);

						string line;
						ifstream manifest("assets\/atlas.manifest");

						int lc = 1; // Line count in the Hold Browser.

						if (manifest.is_open())
						{
							while (!manifest.eof())
							{
								std::getline(manifest, line);

								if (line != "")
								{
									string pathimage = "DDS convert and Atlas\/" + line + ".png";
									Fl_Shared_Image* pngimage = Fl_Shared_Image::get(pathimage.c_str());
									pngimage->scale(32, 32, 1);

									quadBrowser->add(line.c_str());
									quadBrowser->icon(lc, pngimage);
									lc++;
								}
								std::getline(manifest, line); // skip
								std::getline(manifest, line); // skip
								std::getline(manifest, line); // skip
								std::getline(manifest, line); // skip
							}
						}
						manifest.close();

						quadPinch_spinner = new Fl_Spinner(1160, 565, 50, 25, "Pinch Texture");
						quadPinch_spinner->align(FL_ALIGN_RIGHT);
						quadPinch_spinner->callback(transformObject, (void*)false);
						quadPinch_spinner->color(FL_GRAY);
						quadPinch_spinner->value(1);
						quadPinch_spinner->step(1);
						quadPinch_spinner->minimum(-1000);
						quadPinch_spinner->maximum(1000);

						quadToggle_checkbox = new Fl_Check_Button (1160, 530, 25, 25, "Toggle");
						quadToggle_checkbox->align(FL_ALIGN_RIGHT);
						quadToggle_checkbox->callback(transformObject, (void*)false);

						quadRotate_slider = new Fl_Value_Slider(1160, 600, 150, 20, "Rotate");
						quadRotate_slider->type(FL_HORIZONTAL);
						quadRotate_slider->minimum(0);
						quadRotate_slider->maximum(270);
						quadRotate_slider->step(90);
				}
					qta->end();

					Fl_Group* bbb = new Fl_Group(50, 75, 1430, 575, "Ortho Textures"); // Paper design how the two autoTx and multiTx would look and maybe knock this into "orthotex"
					{
						fl_register_images();

						Fl_Group* orthotex_group;
						orthotex_group = new Fl_Group(1055, 100, 400, 545, "Ortho Textures");
						orthotex_group->box(FL_UP_FRAME);
						orthotex_group->labeltype(FL_SHADOW_LABEL);
						orthotex_group->end();

						imgbrowser = new Fl_Hold_Browser(1070, 120, 370, 400);
						imgbrowser->callback(transformObject, (void*)false);

						string line;
						ifstream manifest("assets\/atlas.manifest");

						int lc = 1; // Line count in the Hold Browser.

						if (manifest.is_open())
						{
							while (!manifest.eof())
							{
								std::getline(manifest, line);

								if (line != "")
								{
									string pathimage = "DDS convert and Atlas\/" + line + ".png";
									Fl_Shared_Image* pngimage = Fl_Shared_Image::get(pathimage.c_str());
									pngimage->scale(32, 32, 1);

									imgbrowser->add(line.c_str());
									imgbrowser->icon(lc, pngimage);
									lc++;									
								}
								std::getline(manifest, line); // skip
								std::getline(manifest, line); // skip
								std::getline(manifest, line); // skip
								std::getline(manifest, line); // skip
							}
						}
						manifest.close();

						zoom_counter = new Fl_Float_Input(1200, 530, 90, 25, "Zoom");
						zoom_counter->callback(transformObject, (void*)false);
						zoom_counter->color(FL_GRAY);
						zoom_counter->value("0.00");

						mixratio_slider = new Fl_Value_Slider(1160, 565, 130, 25, "Mix");
						mixratio_slider->minimum(0.00);
						mixratio_slider->maximum(1.00);
						mixratio_slider->callback(transformObject, (void*)false);
						mixratio_slider->type(FL_HORIZONTAL);

						zoffset_counter = new Fl_Float_Input(1200, 610, 90, 25, "Zset");
						zoffset_counter->callback(transformObject, (void*)false);
						zoffset_counter->color(FL_GRAY);
						zoffset_counter->value("0.00");

						zoffset_roller = new Fl_Roller(1310, 530, 25, 95, "Zset");
						zoffset_roller->callback(transformObject, (void*)false);
						zoffset_roller->color(FL_GRAY);
						zoffset_roller->value(0.00);
						zoffset_roller->step(0.5);
						zoffset_roller->minimum(-100);
						zoffset_roller->maximum(100);
					}
					bbb->end();

					Fl_Group* ccc = new Fl_Group(50, 75, 1430, 575, "Material");
					{
						Fl_Group* group1 = new Fl_Group(1075, 100, 225, 332, "Surface Material");
						group1->box(FL_UP_FRAME);
						group1->labeltype(FL_SHADOW_LABEL);
						group1->end();

						//Fl_Round_Button

						Fl_PNG_Image* facet_bulb = new Fl_PNG_Image("assets\/bulb_facet.png");
						light_facet_button = new Fl_Light_Button(1135, 125, 48, 48);
						light_facet_button->type(FL_RADIO_BUTTON);
						light_facet_button->image(facet_bulb);
						light_facet_button->callback(transformObject, (void*)false);

						Fl_PNG_Image* average_bulb = new Fl_PNG_Image("assets\/bulb_average.png");
						light_average_button = new Fl_Light_Button(1185, 125, 48, 48);
						light_average_button->type(FL_RADIO_BUTTON);
						light_average_button->image(average_bulb);
						light_average_button->value(1); // Default value is average.
						light_average_button->callback(transformObject, (void*)false);

						fresnel_slider = new Fl_Value_Slider(1110, 205, 150, 30, "Fresnel");
						fresnel_slider->minimum(0.00);
						fresnel_slider->maximum(6.00);
						fresnel_slider->callback(transformObject, (void*)false);
						fresnel_slider->type(FL_HORIZONTAL);
						envmap_slider = new Fl_Value_Slider(1110, 265, 150, 30, "EnvMap");
						envmap_slider->minimum(0.00);
						envmap_slider->maximum(1.00);
						envmap_slider->callback(transformObject, (void*)false);
						envmap_slider->type(FL_HORIZONTAL);
						alpha_slider = new Fl_Value_Slider(1110, 325, 150, 30, "Alpha");
						alpha_slider->minimum(0.00);
						alpha_slider->maximum(1.00);
						alpha_slider->callback(transformObject, (void*)false);
						alpha_slider->type(FL_HORIZONTAL);
						alpha_slider->value(1.00);

						winding_checkbox = new Fl_Check_Button(1110, 385, 150, 35, "Reverse Winding");
						winding_checkbox->deimage(average_bulb);
						winding_checkbox->callback(transformObject, (void*)false);
						winding_checkbox->selection_color(FL_BLACK);

						Fl_Group* group2 = new Fl_Group(1075, 470, 225, 100, "All Texture Offsets");
						group2->box(FL_UP_FRAME);
						group2->labeltype(FL_SHADOW_LABEL);
						group2->end();

						maintexOffset_checkbox = new Fl_Check_Button(1110, 485, 150, 35, "Use TexCoords");
						maintexOffset_checkbox->callback(transformObject, (void*)false);
						maintexOffset_checkbox->selection_color(FL_BLACK);

						maintexOffset_counter = new Fl_Float_Input(1205, 525, 45, 25, "Pinch Texture");
						maintexOffset_counter->callback(transformObject, (void*)false);
						maintexOffset_counter->color(FL_GRAY);
						maintexOffset_counter->value("1");
					}
					ccc->end();

					Fl_Group* ddd = new Fl_Group(50, 75, 1430, 575, "Wavefront Texture");
					{
						Fl_Group* wavetex_group;
						wavetex_group = new Fl_Group(1055, 100, 400, 545, "Wavefront Texture");
						wavetex_group->box(FL_UP_FRAME);
						wavetex_group->labeltype(FL_SHADOW_LABEL);
						wavetex_group->end();

						wavefront_browser = new Fl_Hold_Browser(1070, 120, 370, 455);
						wavefront_browser->callback(setWavefrontTexture_cb);

						intex_checkbox = new Fl_Check_Button(1170, 575, 150, 35, "Internal TexCoords");
						intex_checkbox->callback(intex_cb, (void*)false);
						intex_checkbox->selection_color(FL_BLACK);

						pinch_counter = new Fl_Float_Input(1260, 610, 45, 25, "Pinch Texture");
						pinch_counter->callback(pinch_cb);
						pinch_counter->color(FL_GRAY);
						pinch_counter->value("1");

						string line;
						ifstream manifest3("assets\/atlas.manifest");

						int lc = 1; // Line count in the Hold Browser.

						if (manifest3.is_open())
						{
							while (!manifest3.eof())
							{
								std::getline(manifest3, line);

								if (line != "")
								{
									string pathimage = "DDS convert and Atlas\/" + line + ".png";
									Fl_Shared_Image* pngimage = Fl_Shared_Image::get(pathimage.c_str());
									pngimage->scale(32, 32, 1);

									wavefront_browser->add(line.c_str());
									wavefront_browser->icon(lc, pngimage);
									lc++;
								}
								std::getline(manifest3, line); // skip
								std::getline(manifest3, line); // skip
								std::getline(manifest3, line); // skip
								std::getline(manifest3, line); // skip
							}
						}
						manifest3.close();
					}
					ddd->end();

					Fl_Group* eee = new Fl_Group(50, 75, 1430, 575, "Retarget Textures");
					{
						Fl_Group* group1 = new Fl_Group(1055, 100, 400, 150, "Current Texture References");
						group1->box(FL_UP_FRAME);
						group1->labeltype(FL_SHADOW_LABEL);
						group1->end();

						current_browser = new Fl_Hold_Browser(1100, 115, 315, 125);
						current_browser->callback(scene_browser_cb);
						current_browser->color(FL_GRAY);

						Fl_PNG_Image* down_image = new Fl_PNG_Image("assets\/down_button.png");
						transferdown_button = new Fl_Button(1100, 257, 60, 30);
						transferdown_button->image(down_image);
						transferdown_button->callback(transferdownTex_cb, (void*)1);

						Fl_Group* group2 = new Fl_Group(1055, 290, 400, 150, "Replacements");
						group2->box(FL_UP_FRAME);
						group2->labeltype(FL_SHADOW_LABEL);
						group2->end();

						replacement_browser = new Fl_Hold_Browser(1100, 303, 315, 125);
						replacement_browser->callback(scene_browser_cb);
						replacement_browser->color(FL_GRAY);

						Fl_PNG_Image* up_image = new Fl_PNG_Image("assets\/up_button.png");
						transferup_button = new Fl_Button(1100, 447, 60, 30);
						transferup_button->image(up_image);
						transferup_button->callback(transferupTex_cb, (void*)1);

						Fl_Group* group3 = new Fl_Group(1055, 480, 400, 150, "Atlas References");
						group3->box(FL_UP_FRAME);
						group3->labeltype(FL_SHADOW_LABEL);
						group3->end();

						atlas_browser = new Fl_Hold_Browser(1100, 500, 315, 125);
						atlas_browser->callback(scene_browser_cb);
						atlas_browser->color(FL_GRAY);

						string line;
						ifstream manifest4("assets\/atlas.manifest");

						if (manifest4.is_open())
						{
							while (!manifest4.eof())
							{
								std::getline(manifest4, line);

								if (line != "")
									atlas_browser->add(line.c_str());

								std::getline(manifest4, line); // skip
								std::getline(manifest4, line); // skip
								std::getline(manifest4, line); // skip
								std::getline(manifest4, line); // skip
							}
						}
						manifest4.close();

						cleartex_button = new Fl_Button(1325, 257, 125, 25, "Clear");
						cleartex_button->callback(clearTex_cb);

						replace_button = new Fl_Button(1325, 447, 125, 25, "Replace");
						replace_button->callback(replaceTex_cb);
					}
					eee->end();

					Fl_Group* fff = new Fl_Group(50, 75, 1430, 575, "HUD");
					{
						Fl_Group* group1 = new Fl_Group(1055, 100, 400, 390, "Rail Settings");
						group1->box(FL_UP_FRAME);
						group1->labeltype(FL_SHADOW_LABEL);
						group1->end();

						rail_browser = new Fl_Hold_Browser(1100, 110, 315, 225);
						rail_browser->callback(rail_cb);
						rail_browser->color(FL_GRAY);

						railwidth_counter = new Fl_Float_Input(1250, 355, 45, 25, "Rail Width");
						railwidth_counter->callback(rail_cb);
						railwidth_counter->color(FL_GRAY);
						railwidth_counter->value("1");

						railgauge_counter = new Fl_Float_Input(1250, 385, 45, 25, "Rail Gauge");
						railgauge_counter->callback(rail_cb);
						railgauge_counter->color(FL_GRAY);
						railgauge_counter->value("1");

						railsteps_counter = new Fl_Float_Input(1250, 415, 45, 25, "Rail Steps");
						railsteps_counter->callback(rail_cb);
						railsteps_counter->color(FL_GRAY);
						railsteps_counter->value("1");

						railsave_button = new Fl_Button(1170, 450, 125, 25, "Save Parameters");
						railsave_button->callback(railsave_cb);

						string line;
						ifstream manifest5("assets\/atlas.manifest");

						if (manifest5.is_open())
						{
							while (!manifest5.eof())
							{
								std::getline(manifest5, line);

								if (line != "")
									rail_browser->add(line.c_str());

								std::getline(manifest5, line); // skip
								std::getline(manifest5, line); // skip
								std::getline(manifest5, line); // skip
								std::getline(manifest5, line); // skip
							}
						}
						manifest5.close();
					}
					fff->end();
				}
				tabs->end();
			}
			obj_group->end();

			scn_group = new Fl_Group(50, 50, 1430, 600, "Scene");
			{
				Fl_Tabs* tabs2 = new Fl_Tabs(50, 50, 1430, 600);
				{
					Fl_Group* aaa2 = new Fl_Group(50, 75, 1430, 600, "Objects");
					{
						Fl_Group* group1 = new Fl_Group(1055, 100, 285, 445, "Objects");
						group1->box(FL_UP_FRAME);
						group1->labeltype(FL_SHADOW_LABEL);
						group1->end();

						scene_browser = new Fl_Hold_Browser(1100, 120, 226, 165);
						scene_browser->callback(scene_browser_cb);
						scene_browser->color(FL_GRAY);

						object_browse_button = new Fl_Button(1135, 296, 125, 25, "Browse..");
						object_browse_button->callback(object_browse_cb);

						object_delete_button = new Fl_Button(1135, 326, 125, 25, "Delete");
						object_delete_button->callback(object_delete_cb);

						Fl_PNG_Image* up_button = new Fl_PNG_Image("assets\/up_button.png");
						scn_up_button = new Fl_Button(1065, 170, 30, 30);
						scn_up_button->image(up_button);
						scn_up_button->callback(scnReorder_cb, (void*)1);

						Fl_PNG_Image* down_button = new Fl_PNG_Image("assets\/down_button.png");
						scn_down_button = new Fl_Button(1065, 200, 30, 30);
						scn_down_button->image(down_button);
						scn_down_button->callback(scnReorder_cb, (void*)2);

						scn_posx_counter = new Fl_Float_Input(1105, 360, 90, 25, "Pos X");
						scn_posx_counter->callback(transformScene);
						scn_posx_counter->color(FL_GRAY);
						scn_posx_counter->value("0.00");

						scn_posy_counter = new Fl_Float_Input(1105, 390, 90, 25, "Pos Y");
						scn_posy_counter->callback(transformScene);
						scn_posy_counter->color(FL_GRAY);
						scn_posy_counter->value("0.00");

						scn_posz_counter = new Fl_Float_Input(1105, 420, 90, 25, "Pos Z");
						scn_posz_counter->callback(transformScene);
						scn_posz_counter->color(FL_GRAY);
						scn_posz_counter->value("0.00");

						scn_scale_counter = new Fl_Float_Input(1235, 360, 90, 25, "Scl X");
						scn_scale_counter->callback(transformScene);
						scn_scale_counter->color(FL_GRAY);
						scn_scale_counter->value("0.00");

						scn_rotx_counter = new Fl_Float_Input(1105, 450, 90, 25, "Rot X");
						scn_rotx_counter->callback(transformScene);
						scn_rotx_counter->color(FL_GRAY);
						scn_rotx_counter->value("0.00");

						scn_roty_counter = new Fl_Float_Input(1105, 480, 90, 25, "Rot Y");
						scn_roty_counter->callback(transformScene);
						scn_roty_counter->color(FL_GRAY);
						scn_roty_counter->value("0.00");

						scn_rotz_counter = new Fl_Float_Input(1105, 510, 90, 25, "Rot Z");
						scn_rotz_counter->callback(transformScene);
						scn_rotz_counter->color(FL_GRAY);
						scn_rotz_counter->value("0.00");

						Fl_Group* group2 = new Fl_Group(1055, 570, 285, 50, "Global Position");
						group2->box(FL_UP_FRAME);
						group2->labeltype(FL_SHADOW_LABEL);
						group2->end();

						scn_gblx_counter = new Fl_Float_Input(1080, 580, 70, 25, "X");
						scn_gblx_counter->callback(transformScene);
						scn_gblx_counter->color(FL_GRAY);
						scn_gblx_counter->value("0.00");

						scn_gbly_counter = new Fl_Float_Input(1170, 580, 70, 25, "Y");
						scn_gbly_counter->callback(transformScene);
						scn_gbly_counter->color(FL_GRAY);
						scn_gbly_counter->value("0.00");

						scn_gblz_counter = new Fl_Float_Input(1260, 580, 70, 25, "Z");
						scn_gblz_counter->callback(transformScene);
						scn_gblz_counter->color(FL_GRAY);
						scn_gblz_counter->value("0.00");

						scn_shape_dropdown = new Fl_Choice(1200, 390, 125, 25, "");
						scn_shape_dropdown->callback(transformScene);
						scn_shape_dropdown->color(FL_GRAY);
						scn_shape_dropdown->add("Cube");
						scn_shape_dropdown->add("Sphere");
						scn_shape_dropdown->add("Capsule");
						scn_shape_dropdown->add("Cylinder");
						scn_shape_dropdown->add("Cone");
						scn_shape_dropdown->add("Plane");
						scn_shape_dropdown->value(0);

						scn_physics_checkbox = new Fl_Check_Button(1200, 420, 125, 25, "Collision View");
						scn_physics_checkbox->callback(transformScene);
						scn_physics_checkbox->selection_color(FL_BLACK);
					}
					aaa2->end();

					Fl_Group* bbb2 = new Fl_Group(50, 75, 1430, 600, "Collision");
					{

					}
					bbb2->end();

					Fl_Group* ccc2 = new Fl_Group(50, 75, 1430, 600, "Cubemap");
					{
						Fl_Group* group1 = new Fl_Group(1055, 100, 385, 300, "Cubemaps");
						group1->box(FL_UP_FRAME);
						group1->labeltype(FL_SHADOW_LABEL);
						group1->end();

						cubemap_browser = new Fl_Hold_Browser(1070, 120, 356, 165);
						cubemap_browser->callback(cubemap_browser_cb);
						cubemap_browser->color(FL_GRAY);

						string line;
						ifstream cmmanifest("assets\/cubemaps\/cubemap_index.txt"); // load the cubemap_index.txt or something.

						CubemapPair cbp;

						if (cmmanifest.is_open())
						{
							while (!cmmanifest.eof())
							{
								std::getline(cmmanifest, line);

								if (line != "")
								{
									cubemap_browser->add(line.c_str()); // figure out the parse here.
									cbp.name = line;
								}

								std::getline(cmmanifest, line);
								cbp.path = line;

								if ( line != "") // Ignore the line ending, if there is one.
									cubemapData.push_back(cbp);
							}
						}
						cmmanifest.close();

						scn_cubemap_button = new Fl_Button(1275, 300, 150, 25, "Create Cubemap");
						scn_cubemap_button->callback(cubemap_cb);

						scn_cubemap_delete_button = new Fl_Button(1275, 360, 150, 25, "Delete Cubemap");
						scn_cubemap_delete_button->color(88 + 1);
						scn_cubemap_delete_button->callback(delete_cubemap_cb);

						cubemapInput = new Fl_Input(1115, 300, 150, 25, "Name:");
						cubemapInput->color(FL_GRAY);
					}
					ccc2->end();
				}
				tabs2->end();
			}
			scn_group->end();

			cam_group = new Fl_Group(50, 50, 1430, 600, "Camera");
			{
				Fl_Tabs* tabs3 = new Fl_Tabs(50, 50, 1430, 600);
				{
					Fl_Group* aaa3 = new Fl_Group(50, 75, 1430, 600, "Settings");
					{
						ortho_checkbox = new Fl_Check_Button(1050, 100, 150, 35, "Ortho Mode");
						ortho_checkbox->callback(ortho_cb, (void*)false);
						ortho_checkbox->selection_color(FL_BLACK);

						backface_checkbox = new Fl_Check_Button(1050, 140, 190, 35, "See Back-faces");
						backface_checkbox->callback(backface_cb, (void*)false);
						backface_checkbox->selection_color(FL_BLACK);

						wireframe_checkbox = new Fl_Check_Button(1050, 180, 190, 35, "See Wireframe");
						wireframe_checkbox->callback(wireframe_cb, (void*)false);
						wireframe_checkbox->selection_color(FL_BLACK);

						originx_checkbox = new Fl_Check_Button(1050, 220, 100, 35, "See Origin X");
						originx_checkbox->callback(origin_cb, (void*)false);
						originx_checkbox->selection_color(FL_BLACK);

						originy_checkbox = new Fl_Check_Button(1160, 220, 30, 35, "Y");
						originy_checkbox->callback(origin_cb, (void*)false);
						originy_checkbox->selection_color(FL_BLACK);

						originz_checkbox = new Fl_Check_Button(1200, 220, 30, 35, "Z");
						originz_checkbox->callback(origin_cb, (void*)false);
						originz_checkbox->selection_color(FL_BLACK);

						origin_slider = new Fl_Value_Slider(1250, 225, 130, 25, "Origin Alpha");
						origin_slider->minimum(0.00);
						origin_slider->maximum(1.00);
						origin_slider->value(0.5);
						origin_slider->callback(origin_cb, (void*)false);
						origin_slider->type(FL_HORIZONTAL);

						camera_counter = new Fl_Float_Input(1150, 270, 90, 25, "Camera Delta");
						camera_counter->callback(cameraDelta);
						camera_counter->color(FL_GRAY);
						camera_counter->value("250.0");
					}
					aaa3->end();
				}
			}
			cam_group->end();

			shaders_group = new Fl_Group(50, 50, 1430, 600, "Shaders");
			{
				Fl_Tabs* tabs4 = new Fl_Tabs(50, 50, 1430, 600);
				{
					Fl_Group* aaa4 = new Fl_Group(50, 75, 1430, 600, "Vertex");
					{
						vertexbuff = new Fl_Text_Buffer();
						vertex_editor = new Fl_Text_Editor(1030, 77, 450, 505);
						vertex_editor->buffer(vertexbuff);

						int r = vertexbuff->insertfile("assets\/arena_vertex.txt", 0);

						vertex_editor->textfont(FL_COURIER);
						vertex_editor->textsize(11);
						vertex_editor->color(FL_GRAY);
						vertex_editor->linenumber_width(30);
						vertex_editor->linenumber_size(11);

						vertex_compile_button = new Fl_Button(1060, 590, 75, 30, "Compile");
						vertex_compile_button->callback(vertex_compile_cb);

						vertex_reset_button = new Fl_Button(1280, 590, 75, 30, "Reset");
						vertex_reset_button->callback(vertex_reset_cb);
						vertex_save_button = new Fl_Button(1370, 590, 75, 30, "Save");
						vertex_save_button->callback(vertex_save_cb);
					}
					aaa4->end();

					Fl_Group* bbb4 = new Fl_Group(50, 75, 1430, 600, "Fragment");
					{
						fragmentbuff = new Fl_Text_Buffer();
						fragment_editor = new Fl_Text_Editor(1030, 77, 450, 505);
						fragment_editor->buffer(fragmentbuff);

						int r = fragmentbuff->insertfile("assets\/arena_fragment.txt", 0);

						fragment_editor->textfont(FL_COURIER);
						fragment_editor->textsize(11);
						fragment_editor->color(FL_GRAY);
						fragment_editor->linenumber_width(30);
						fragment_editor->linenumber_size(11);

						fragment_compile_button = new Fl_Button(1060, 590, 75, 30, "Compile");
						fragment_compile_button->callback(fragment_compile_cb);

						fragment_reset_button = new Fl_Button(1280, 590, 75, 30, "Reset");
						fragment_reset_button->callback(fragment_reset_cb);
						fragment_save_button = new Fl_Button(1370, 590, 75, 30, "Save");
						fragment_save_button->callback(fragment_save_cb);
					}
					bbb4->end();
				}
			}
			shaders_group->end();
		}
		mtabs1->callback(switchTab, NULL);
		mtabs1->end();

		quadCountbuff = new Fl_Text_Buffer();
		quadCount_display = new Fl_Text_Display(1100, 25, 250, 25);
		quadCount_display->color(FL_GRAY);
		quadCount_display->buffer(quadCountbuff);
		quadCountbuff->text("Quads: ");

		Fl::add_timeout(0.05, timer_cb); // Handles real-time mouse and keyboard
	}
};
