#include "MRPointsLoad.h"
#include "MRPointsLoadSettings.h"

#include "detail/TypeCast.h"
#include "detail/Vector.h"

#include "MRMesh/MRColor.h"
#include "MRMesh/MRPointCloud.h"
#include "MRMesh/MRPointsLoad.h"

#include <cstring>

using namespace MR;

REGISTER_AUTO_CAST( PointCloud )
REGISTER_AUTO_CAST( PointsLoadSettings )
REGISTER_AUTO_CAST2( std::string, MRString )

MRTextFileAnalysisResult* mrPointsAnalyzeText( const char* filename, MRString** errorString )
{
    auto res = PointsLoad::analyzeText( filename );

    if ( res )
    {
        auto* result = new MRTextFileAnalysisResult;
        result->header = strdup( res->header.c_str() );
        result->firstDataLine = strdup( res->firstDataLine.c_str() );
        result->totalLines = res->totalLines;
        result->commentLines = res->commentLines;
        result->emptyLines = res->emptyLines;
        result->dataLines = res->dataLines;
        result->hasNormals = res->hasNormals;
        result->hasColors = res->hasColors;
        return result;
    }
    else
    {
        if ( errorString )
            *errorString = auto_cast( new_from( std::move( res.error() ) ) );
        return NULL;
    }
}

void mrTextFileAnalysisResultFree( MRTextFileAnalysisResult* result )
{
    if ( result )
    {
        free( (void*)result->header );
        free( (void*)result->firstDataLine );
        delete result;
    }
}

MRPointCloud* mrPointsLoadFromAnySupportedFormat( const char* filename, const MRPointsLoadSettings* settings_, MRString** errorString )
{
    PointsLoadSettings settings;
    if ( settings_ )
    {
        if ( settings_->colors )
        {
            vector_wrapper<Color>* wrapper = reinterpret_cast<vector_wrapper<Color>* >( settings_->colors );
            if ( wrapper )
            {
                auto& colors = (std::vector<Color>&)( *wrapper );
                settings.colors = reinterpret_cast< VertColors* >( &colors );
            }
        }
        settings.outXf = ( AffineXf3f* )settings_->outXf;
        settings.callback = settings_->callback;
    }

    auto res = PointsLoad::fromAnySupportedFormat( filename, settings );

    if ( res )
    {
        if ( settings.colors )
            mrVertColorsInvalidate( settings_->colors );

        RETURN_NEW( std::move( *res ) );
    }
    else
    {
        if ( errorString )
            *errorString = auto_cast( new_from( std::move( res.error() ) ) );
        return NULL;
    }
}
