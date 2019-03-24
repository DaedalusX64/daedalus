#include "stdafx.h"

#include "TextureCache.h"

#include "SysOSX/Debug/WebDebug.h"
#include "SysOSX/Debug/WebDebugTemplate.h"

#include "Utility/Mutex.h"
#include "Utility/DataSink.h"
#include "Graphics/PngUtil.h"


#ifdef DAEDALUS_DEBUG_DISPLAYLIST
static void TextureHandler(void * arg, WebDebugConnection * connection)
{
	const char * params = connection->GetQueryString();
	if (!params)
		return;

	// Yes, this is pretty dodgy. Any random pointer can be passed in so we
	// validated ptr against the list of textures below.
	void * ptr {};
	if (sscanf(params, "ptr=%p", &ptr) != 1)
	{
		printf("Couldn't parse pointer: %s\n", params);
		return;
	}

	// NB: maintain a lock for as long as we have a ref to any textures.
	// If we delete textures on this thread, we'll crash OpenGL.
	{
		MutexLock lock(CTextureCache::Get()->GetDebugMutex());
		CRefPtr<CNativeTexture> texture;
		{
			std::vector<CTextureCache::STextureInfoSnapshot> textures;
			CTextureCache::Get()->Snapshot(lock, textures);
			for (size_t i {}; i < textures.size(); ++i)
			{
				CTextureCache::STextureInfoSnapshot & snap = textures[i];
				if ((CNativeTexture*)snap.Texture == ptr)
				{
					texture = snap.Texture;
					break;
				}
			}
		}

		// Return a 404 if not found
		if (!texture)
		{
			printf("Couldn't find texture %p\n", ptr);
			return;
		}

		connection->BeginResponse(200, -1, "image/png");
		PngSaveImage(connection, texture);
	}

	connection->EndResponse();
}
#endif // DAEDALUS_DEBUG_DISPLAYLIST


#ifdef DAEDALUS_DEBUG_DISPLAYLIST
static void TextureCacheHandler(void * arg, WebDebugConnection * connection)
{
	connection->BeginResponse(200, -1, "text/html" );

	WriteStandardHeader(connection, "Texture Cache");

	connection->WriteString(
		"<div class=\"container\">\n"
		"	<div class=\"row\">\n"
		"		<div class=\"span12\">\n"
	);
	connection->WriteString("<h1>Texture Cache</h1>\n");
	connection->WriteString("<table class=\"table table-condensed\">");
	connection->WriteString("<thead>");

	connection->WriteF(
		"<tr>"
		"<th>Format</th>"
		"<th>Pitch</th>"
		"<th>Width</th>"
		"<th>Height</th>"
		"<th>Image</th>"
		"</tr>"
		"\n" );

	connection->WriteString("</thead>");
	connection->WriteString("<tbody>");

	// NB: maintain a lock for as long as we have a ref to any textures.
	// If we delete textures on this thread, we'll crash OpenGL.
	{
		MutexLock lock(CTextureCache::Get()->GetDebugMutex());

		std::vector<CTextureCache::STextureInfoSnapshot> textures;
		CTextureCache::Get()->Snapshot(lock, textures);

		for (size_t i {}; i < textures.size(); ++i)
		{
			CTextureCache::STextureInfoSnapshot & snap = textures[i];
			const TextureInfo & ti = snap.Info;

			connection->WriteF(
				"<tr>"
				"<td>%s/%dbpp</td>"
				"<td>%d</td>"
				"<td>%d</td>"
				"<td>%d</td>"
				"<td><a href=\"/texture?ptr=%p\"><img src=\"/texture?ptr=%p\" width=%d height=%d></a></td>"
				"</tr>"
				"\n",
				ti.GetFormatName(),
				ti.GetSizeInBits(),
				ti.GetPitch(),
				ti.GetWidth(),
				ti.GetHeight(),
				(CNativeTexture *)snap.Texture,
				(CNativeTexture *)snap.Texture,
				snap.Texture->GetWidth(),
				snap.Texture->GetHeight()
			);
		}
	}

	connection->WriteString("</tbody>");
	connection->WriteString("</table>");

	connection->WriteString(
		"		</div>\n"
		"	</div>\n"
		"</div>\n"
	);

	WriteStandardFooter(connection);
	connection->EndResponse();
}
#endif // DAEDALUS_DEBUG_DISPLAYLIST

bool TextureCache_RegisterWebDebug()
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	WebDebug_Register( "/texture_cache", &TextureCacheHandler, NULL );
	WebDebug_Register( "/texture", &TextureHandler, NULL );
#endif
	return true;
}
