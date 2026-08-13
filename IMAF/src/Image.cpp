#include "IMAF/Image.h"
#include "IMAF/Application.h"
#include "IMAF/GLLoader.h"
#include "IMAF/Textures.h"

#include "GLES3/gl3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#undef LoadImage

#include <unordered_set>
#include <imgui.h>
#include <gl/GL.h>
#include <string_view>
#include <filesystem>
#include <Windows.h>
#include <string>
#include <algorithm>
#include <cctype>

#undef LoadImage

static const std::unordered_set<std::string_view> s_supported_extensions(
	{ 
		".png",".jpg",".jpeg",".bmp",".tga",".gif",".psd",".hdr",".pic",
	}, 9);

IMAF::Image::Image(const char* path) :
	m_error(false), m_path(path), m_data(nullptr), m_width(0), m_height(0), m_from_buffer(false), m_texture_data(0), m_tint(IM_COL32_WHITE), m_start_pos(0,0), m_gl_context(nullptr)
{
	if (path == nullptr)
	{
		m_error = true;
		return;
	}

	if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
	{
		m_error = true;
		return;
	}
	
	std::string extension = std::filesystem::path(m_path).extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });
	if (!s_supported_extensions.contains(extension))
	{
		m_error = true;
		return;
	}

	if (!LoadImage())
		m_error = true;
}

IMAF::Image::Image(const unsigned char* buffer, int size) :
	m_error(false), m_data(nullptr), m_width(0), m_height(0), m_from_buffer(true), m_buffer(buffer), m_buffer_size(size), m_texture_data(0), m_tint(IM_COL32_WHITE), m_start_pos(0, 0), m_gl_context(nullptr)
{
	if (buffer == nullptr || size == 0)
	{
		m_error = true;
		return;
	}
	
	if (!LoadImage())
		m_error = true;
}

ImVec2 IMAF::Image::GetSize() const
{
	return ImVec2(m_width,m_height);
}

unsigned char* IMAF::Image::GetRawData()
{
	return m_data;
}

bool IMAF::Image::Error() const
{
	return m_error;
}

void IMAF::Image::SetImagePosition(const ImVec2& start_pos)
{
	m_start_pos = start_pos;
}

void IMAF::Image::SetImageTint(ImU32 tint_color)
{
	m_tint = tint_color;
}

void IMAF::Image::DisplayImGuiImage(const ImVec2& img_size, const ImVec2& start_point, const ImVec2& end_point, bool use_position)
{
	if (m_error)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, RGB2_IMVEC4(230,30,30));
		ImGui::Text("Error loading image");
		ImGui::PopStyleColor();
		return;
	}

	if (m_texture_data == 0)
	{
		m_gl_context = wglGetCurrentContext();

		if (IMAF::Utils::LoadGlFunctions())
		{
			MessageBoxA(NULL, "Failed to load opengl function.", "Critical Error", MB_ICONERROR);
			ExitProcess(1);
		}

		GLint last_texture = 0;
		GLint last_unpack_row_length = 0;
		GLint last_unpack_buffer = 0;

		glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
		glGetIntegerv(GL_UNPACK_ROW_LENGTH, &last_unpack_row_length);
		glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &last_unpack_buffer);

		// Create a OpenGL texture identifier
		glGenTextures(1, &m_texture_data);
		glBindTexture(GL_TEXTURE_2D, m_texture_data);

		// Setup filtering parameters for display
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

		IMAF::Utils::glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_data);

		// Restore previous OpenGL state
		glPixelStorei(GL_UNPACK_ROW_LENGTH, last_unpack_row_length);
		IMAF::Utils::glBindBuffer(GL_PIXEL_UNPACK_BUFFER, last_unpack_buffer);
		glBindTexture(GL_TEXTURE_2D, last_texture);
	}
	
	if (img_size.x < 0 || img_size.y < 0)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, RGB2_IMVEC4(225, 30, 30));
		ImGui::Text("Error displaying image: invalid image size");
		ImGui::PopStyleColor();
		return;
	}

	float x_factor = img_size.x == 0 ? 1 : img_size.x / m_width;
	float y_factor = img_size.y == 0 ? 1 : img_size.y / m_height;

	ImVec2 end_pos{ 0,0 };
	if (use_position)
	{
		if (img_size.x > 0 && img_size.y > 0)
		{
			end_pos.x = m_start_pos.x + img_size.x;
			end_pos.y = m_start_pos.y + img_size.y;
		}
		else
		{
			end_pos.x = m_start_pos.x + m_width;
			end_pos.y = m_start_pos.y + m_height;
		}
	}

	if (start_point.x == 0 && start_point.y == 0 && end_point.x == -1 && end_point.y == -1)
	{
		if (!use_position)
			ImGui::Image((void*)(intptr_t)m_texture_data, ImVec2(m_width * x_factor, m_height * y_factor), ImVec2(0,0), ImVec2(1,1), ImGui::ColorConvertU32ToFloat4(m_tint));
		else
			ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)m_texture_data, m_start_pos, end_pos, ImVec2(0, 0), ImVec2(1, 1), m_tint);
	}
	else
	{
		if (start_point.x < 0 || start_point.x > m_width || start_point.y < 0 || start_point.y > m_height || end_point.x < 0 || end_point.x > m_width || end_point.y < 0 || end_point.y > m_height)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, RGB2_IMVEC4(225, 30, 30));
			ImGui::Text("Error displaying image: invalid start or end points");
			ImGui::PopStyleColor();
			return;
		}

		ImVec2 uv0 = ImVec2(start_point.x == 0 ? 0 : start_point.x / m_width, start_point.y == 0 ? 0 : start_point.y / m_height);
		ImVec2 uv1 = ImVec2(end_point.x == 0 ? 0 : end_point.x / m_width, end_point.y == 0 ? 0 : end_point.y / m_height);

		if (!use_position)
			ImGui::Image((void*)(intptr_t)m_texture_data, ImVec2(m_width * x_factor, m_height * y_factor), uv0, uv1, ImGui::ColorConvertU32ToFloat4(m_tint));
		else
			ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)m_texture_data, m_start_pos, end_pos, uv0, uv1, m_tint);
	}
}

IMAF::Image::~Image()
{
	if (m_data != nullptr)
		stbi_image_free(m_data);

	if (m_texture_data != 0 && m_gl_context != nullptr)
		IMAF::Utils::Textures::QueueDelete(m_gl_context, m_texture_data);
}

bool IMAF::Image::LoadImage()
{
	if (m_from_buffer)
	{
		if (m_buffer_size < 0 || m_buffer == nullptr)
			return false;
		m_data = stbi_load_from_memory(m_buffer, m_buffer_size, &m_width, &m_height, nullptr, 4);
	}
	else
		m_data = stbi_load(m_path, &m_width, &m_height, nullptr, 4);

	if (m_data == nullptr)
		return false;

	return true;
}

void IMAF::DisplayImGuiImage(Image& image, const ImVec2& img_size, const ImVec2& start_point, const ImVec2& end_point)
{
	image.DisplayImGuiImage(img_size, start_point, end_point);
}