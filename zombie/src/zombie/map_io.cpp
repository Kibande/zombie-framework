
#include "zombie.hpp"

namespace zombie
{
    MapLoader::MapLoader( datafile::SDStream& sds )
            : sds(sds)
    {
    }

    void MapLoader::Init()
    {
        if (!sds.Init())
        {
            Sys::RaiseException(EX_DATAFILE_ERR, "MapLoader::Init", "Failed to open map file.");
        }
    }

    void MapLoader::Exit()
    {
    }

    void MapLoader::LoadMap(IMapLoadListener* listener)
    {
        bool checked = false;

        while (sds.FindSection())
        {
            uint32_t numId1, numId2;

            if (sds.GetSectionInfo(numId1, numId2))
            {
                Reference<InputStream> section = sds.OpenSection();

                if (numId1 == SDS_ZOMBMAP_HDR)
                {
                    if (numId2 != 10)
                    {
                        Sys::RaiseException(EX_DATAFILE_FORMAT_ERR, "MapLoader::LoadMap", "Unknown map version");
                    }

                    listener->OnMapBegin("");
                }
                else if (numId1 == SDS_ZOMBMAP_EDITOR)
                {
                    glm::vec3 viewPos;
                    float zoom;

                    if ( !section->readItems(&viewPos, 1) || !section->readItems<float>(&zoom, 1) )
                    {
                        Sys::RaiseException(EX_DATAFILE_FORMAT_ERR, "MapLoader::LoadMap", "Data file error: SDS_ZOMBMAP_EDITOR section corrupted");
                    }

                    listener->OnMapEditorData(viewPos, zoom);
                }
                else if (numId1 == SDS_ZOMBMAP_ENTITIES)
                {
                    while ( true )
                    {
                        WorldEnt ent;
                        int16_t type = -1;

                        ent.Init();

                        if ( !section->readItems(&type, 1) )
                            break;
                        
                        if ( !section->readItems(&ent.pos, 1) || !section->readItems(&ent.size, 1) || !section->readItems(&ent.colour, 1) )
                            Sys::RaiseException(EX_DATAFILE_FORMAT_ERR, "MapLoader::LoadMap", "Data file error: SDS_ZOMBMAP_ENTITIES section corrupted");

                        ent.type = type;

                        listener->OnMapEntity( ent );
                    }
                }
            }
        }
    }

    MapWriter::MapWriter( datafile::SDSWriter& sw )
            : sw(sw), section(nullptr)
    {
    }

    void MapWriter::EditorData( const glm::vec3& viewPos, float zoom )
    {
        section = sw.StartSection(SDS_ZOMBMAP_EDITOR, 0);
        section->write<>( viewPos );
        section->write<float>( zoom );
        sw.FlushSection();
    }

    void MapWriter::EntitiesBegin()
    {
        section = sw.StartSection(SDS_ZOMBMAP_ENTITIES, 0);
    }

    void MapWriter::EntitiesAdd( const WorldEnt& ent )
    {
        section->write<int16_t>( ent.type );

        switch ( ent.type )
        {
            case ENT_CUBE:
            case ENT_PLANE:
                section->write<>( ent.pos );
                section->write<>( ent.size );
                section->write<>( ent.colour );
                break;
        }
    }

    void MapWriter::EntitiesEnd()
    {
        sw.FlushSection();
    }

    void MapWriter::Exit()
    {
    }

    void MapWriter::Init( const char* mapName )
    {
        section = sw.StartSection(datafile::SDS_COMMON_ID, datafile::SDS_ID_PURPOSE);
        section->writeString("zombmap");
        sw.FlushSection();

        section = sw.StartSection(SDS_ZOMBMAP_HDR, 10);
        sw.FlushSection();
    }
}