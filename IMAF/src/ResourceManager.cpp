#include "IMAF/ResourceManager.h"
#include <Windows.h>
#include <malloc.h>
#include <string.h>

// ISSUE 2: Resource state is not fully initialized, and GetResource calculates but never stores the resource size.
//FIXED
IMAF::ResourceManager::ResourceManager(int resc_id, const wchar_t* resc_type) : m_id(resc_id),m_type(resc_type),mp_resc(nullptr),m_size(0)
{
	if (GetResource())
		m_error = true;
}

IMAF::ResourceManager::~ResourceManager()
{
	if (m_raii && !m_error)
		Free();
}

void* IMAF::ResourceManager::Get()
{
	return mp_resc;
}

unsigned long IMAF::ResourceManager::Size() const
{
	return m_size;
}

bool IMAF::ResourceManager::Error() const
{
	return m_error;
}

// ISSUE 3: Free does not clear the pointer or ownership state, so an explicit Free followed by destruction double-frees the allocation.
//FIXED
void IMAF::ResourceManager::Free()
{
	if (mp_resc == nullptr)
		return;
	free(mp_resc);
	mp_resc = nullptr;
	m_size = 0;
}

void IMAF::ResourceManager::Owned(bool state)
{
	m_raii = state;
}

bool IMAF::ResourceManager::GetResource()
{
	HMODULE hModule = GetModuleHandleW(NULL);
	if (hModule == NULL)
		return true;

	HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCEW(m_id), m_type);
	if (hRes == NULL)
	{
		DWORD err = GetLastError();
		return true;
	}

	HGLOBAL hMem = LoadResource(NULL, hRes);
	if (hMem == NULL)
		return true;

	m_size = SizeofResource(NULL, hRes);
	LPCVOID lpResc = LockResource(hMem);
	if (lpResc == nullptr)
		return true;

	mp_resc = malloc(m_size);
	if (mp_resc == nullptr)
		return true;
	memcpy(mp_resc, lpResc, m_size);
	
	return false;
}
