#pragma once
#include <filesystem>
#include <string>
#include "imgui.h"

namespace IMAF
{
	class Image
	{
	public:
		Image(const char* path);
		Image(const unsigned char* buffer, size_t size);

		Image(const Image&) = delete;
		Image(Image&&) = delete;
		Image& operator=(const Image&) = delete;
		Image& operator=(Image&&) = delete;

		ImVec2 GetSize() const;

		unsigned char* GetRawData() const;
		bool Error() const;

		void SetImagePosition(const ImVec2& start_pos);
		void SetImageTint(ImU32 tint_color);
		void DisplayImGuiImage(const ImVec2& img_size = ImVec2(0,0), const ImVec2& interimage_start = ImVec2(0, 0), const ImVec2& interimage_end = ImVec2(-1, -1), bool use_position = false);

		~Image();

	private:
		bool LoadImage();

		union
		{
			const char* m_path;
			struct
			{
				const unsigned char* m_buffer;
				size_t m_buffer_size;
			};
		};

		bool m_error;
		bool m_from_buffer;
		unsigned char* m_data;
		unsigned int m_texture_data;
		int m_width;
		int m_height;
		ImU32 m_tint;
		ImVec2 m_start_pos;
	};

	void DisplayImGuiImage(Image& image, const ImVec2& img_size = ImVec2(0, 0), const ImVec2& start_point = ImVec2(0, 0), const ImVec2& end_point = ImVec2(-1, -1));
}