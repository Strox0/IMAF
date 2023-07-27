#pragma once

namespace IMAF {
	
	class ResourceManager
	{
	public:
		explicit ResourceManager(int resc_id,const wchar_t* resc_type);
		~ResourceManager();

		void* Get();
		unsigned long Size();

		bool Error();
		
		void Free();

		//wether the resource memory allocated is own by the class and should be destroyed
		void Owned(bool state);

		ResourceManager(const ResourceManager& other) = delete;
		ResourceManager(ResourceManager&& other) = delete;
	private:
		bool GetResource();
	private:
		bool m_raii = true;
		int m_id;
		const wchar_t* m_type;
		void* mp_resc;
		unsigned long m_size;
		bool m_error = false;
	};

}