/*************************************************************************** 
 * December 2003
 *
 * This TerraPage loader was re-written in a fashion to use PagedLOD 
 * to manage paging entirely, also includes a version of Terrex's smart mesh 
 * adapted to work with PagedLOD. The essential code by Boris Bralo is still present, 
 * slight modified.
 * nick at terrex dot com
 * 
 * Ported to PagedLOD technology by Trajce Nikolov (Nick) & Robert Osfield
 *****************************************************************************/

/***************************************************************************
 * OpenSceneGraph loader for Terrapage format database 
 * by Boris Bralo 2002
 *
 * based on/modifed  sgl (Scene Graph Library) loader by Bryan Walsh
 *
 * This loader is based on/modified from Terrain Experts Performer Loader,
 * and was ported to SGL by Bryan Walsh / bryanw at earthlink dot net
 *
 * That loader is redistributed under the terms listed on Terrain Experts
 * website (www.terrex.com/www/pages/technology/technologypage.htm)
 *
 * "TerraPage is provided as an Open Source format for use by anyone...
 * We supply the TerraPage C++ source code free of charge.  Anyone
 * can use it and redistribute it as needed (including our competitors).
 * We do, however, ask that you keep the TERREX copyrights intact."
 *
 * Copyright Terrain Experts Inc. 1999.
 * All Rights Reserved.
 *
 *****************************************************************************/

#ifndef __TXPPARSER_H_
#define __TXPPARSER_H_

#include <osg/Referenced>
#include <osg/Texture2D>
#include <osg/StateSet>
#include <osg/Node>

#include <stack>

#include "trpage_read.h"

namespace txp
{

// Gets local texture via the image helper
osg::Texture2D* getLocalTexture(trpgrImageHelper& image_helper, const trpgTexture* tex);
osg::Texture2D* getTemplateTexture(trpgrImageHelper& image_helper, trpgLocalMaterial* locmat, const trpgTexture* tex, int index=0);



// This is group that will has geode node
// It is better since all the geometry children will be
// added into one Geode node as drawables, then having one
// geode node per child
// Means, instad of having
// Group
//    +-----------
//    |            |
//    Geode        Geode
//    |            |
//    Drawable    Drawable
// we will have
// Group
//    |
//    Geode
//    +-----------
//    |            |
//    Drawable    Drawable

class GeodeGroup : public osg::Group
{
public:

    GeodeGroup() : osg::Group(), _geode(NULL)
    {};
    
    GeodeGroup(const GeodeGroup& gg,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
    osg::Group(gg, copyop), _geode(gg._geode)
    {};
    
    META_Node(txp, GeodeGroup);
    
    osg::Geode* getGeode()
    {
        if (_geode == 0)
        {   
            _geode = new osg::Geode();   
            addChild(_geode);
        }
        
        return _geode;
    }
protected:
    osg::Geode*    _geode;
};


class TXPArchive;
struct DefferedLightAttribute;

class TXPParser : public trpgSceneParser, public osg::Referenced
{
public:
    TXPParser();
    
    // Sets the archive to be parsed
    inline void setArchive(TXPArchive* archive)
    {
        _archive = archive;
    }
    
    // Scene parser
    osg::Group *parseScene(
        trpgReadBuffer &buf,
        std::vector<osg::ref_ptr<osg::StateSet> > &materials,
        std::vector<osg::ref_ptr<osg::Node> > &models,
        double realMinRange, double realMaxRange, double usedMaxRange);
        
    // Returns the current Top Group
    inline osg::Group* getCurrTop()
    {
        return _currentTop ? _currentTop : _root.get();
    }
    
    // Sets the group as potentional tile group
    inline void setPotentionalTileGroup(osg::Group* grp)
    {
        _tileGroups[grp] = 1;
    }
    
    // Return the current material list (passed in to ParseScene())
    inline std::vector<osg::ref_ptr<osg::StateSet> >* getMaterials()
    {
        return _materials;
    }
    
    // New to TerraPage 2.0 - local materials
    std::vector<osg::ref_ptr<osg::StateSet> >* getLocalMaterials()
    {
        return &_localMaterials;
    }
    
    // Load local materials
    void loadLocalMaterials();
    
    // Return the current model list
    std::vector<osg::ref_ptr<osg::Node> >* getModels()
    {
        return _models;
    }
    
    // Request a model to be load
    bool requestModel(int ix);
    
    // Return a reference to the tile header (after a tile has been read)
    inline trpgTileHeader *getTileHeaderRef()
    {
        return &_tileHeader;
    }
    
    
    // Returns true if we are under billboard subgraph
    inline const bool underBillboardSubgraph() const
    {
        return _underBillboardSubgraph;
    }
    
    // Sets if we are under billboard subgraph
    inline void setUnderBillboardSubgraph(bool b)
    {
        _underBillboardSubgraph = b;
    }
    
    // TXP Billboard info
    struct TXPBillboardInfo
    {
        int type;
        int mode;
        trpg3dPoint center;
        trpg3dPoint axis;
    };
    
    // Sets info for the last billboard parsed
    inline void setLastBillboardInfo(TXPBillboardInfo& info)
    {
        _lastBillboardInfo = info;
    }
    
    // Gets info for the last billboard parsed
    inline void getLastBillboardInfo(TXPBillboardInfo& info)
    {
        info = _lastBillboardInfo;
    }
    
    // Gets light attrib
    DefferedLightAttribute& getLightAttribute(int ix);
    
    // Returns if we are under layer subgraph
    inline const bool underLayerSubgraph() const
    {
        return _underLayerSubgraph;
    }
    
    // Sets if we are under layer subgraph
    inline void setUnderLayerSubgraph(bool b)
    {
        _underLayerSubgraph = b;
    }
    
    // Set the current layer geode
    inline void setLayerGeode(osg::Geode* layer)
    {
        _layerGeode = layer;
    }
    
    // Returns the current layer geode
    inline osg::Geode* getLayerGeode() const
    {
        return _layerGeode;
    }
    
    // default value to use when setting up textures.
    void setMaxAnisotropy(float anisotropy)
    {
        _defaultMaxAnisotropy = anisotropy;
    }
    float getMaxAnisotropy() const
    {
        return _defaultMaxAnisotropy;
    }

    // We need to change the LOD ranges within tiles since
    // we have done it for the PagedLODs representing tiles
    inline double checkAndGetMinRange(double range)
    {
        if ((range-_realMinRange) < 0.0001)
            return 0.0;
        else
            return range;
    }
    inline double checkAndGetMaxRange(double range)
    {
        if ((range-_realMaxRange) < 0.0001)
            return _usedMaxRange;
        else
            return range;
    }
    
    
protected:

    virtual ~TXPParser();
    
    // Removes any empty groups and LODs
    void removeEmptyGroups();
    
    // Called on start children
    bool StartChildren(void *);
    
    // Called on end children
    bool EndChildren(void *);
    
    // THE archive
    TXPArchive    *_archive;
    
    // Current parent
    osg::Group* _currentTop;
    
    // The root of the tile
    osg::ref_ptr<osg::Group> _root;
    
    // Parents list
    std::stack<osg::Group*>    _parents;
    
    // Potentional Tile groups
    std::map<osg::Group*,int>    _tileGroups;
    
    // Replace the tile lod to regular group
    void replaceTileLod(osg::Group*);
    
    // Materials
    std::vector<osg::ref_ptr<osg::StateSet> >*    _materials;
    
    // Local materials
    std::vector<osg::ref_ptr<osg::StateSet> >    _localMaterials;
    
    // Model list
    std::vector<osg::ref_ptr<osg::Node> >*        _models;
    
    // Tile header
    trpgTileHeader    _tileHeader;
    
    // true if we are under billboard subgraph
    bool _underBillboardSubgraph;
    
    // Number of levels below the billboard node
    int _numBillboardLevels;
    
    // Last billboard we parsed
    TXPBillboardInfo _lastBillboardInfo;
    
    // true if we are under layer subgraph
    bool _underLayerSubgraph;
    
    // Numbers of levels below layer subgraph
    int _numLayerLevels;
    
    // Our Layer Geode
    osg::Geode*    _layerGeode;
    
    // default value to use when setting up textures.
    float _defaultMaxAnisotropy;
    
    // LOD ranges to be used when parsing tiles
    double _realMinRange;
    double _realMaxRange;
    double _usedMaxRange;
    
    // TEMP
    osg::Geode* createBoundingBox(int x,int y, int lod);
    
};


//! callback functions for various scene graph elements
//----------------------------------------------------------------------------
class geomRead : public trpgr_Callback
{
public:

    geomRead(TXPParser *in_parse) : _parse(in_parse)
    {};
    void *Parse(trpgToken tok,trpgReadBuffer &buf);
protected:
    TXPParser *_parse;
};


//----------------------------------------------------------------------------
class groupRead : public trpgr_Callback
{
public:
    groupRead(TXPParser *in_parse) : _parse(in_parse)
    {};
    void *Parse(trpgToken tok,trpgReadBuffer &buf);
protected:
    TXPParser *_parse;
};

//----------------------------------------------------------------------------
class lodRead : public trpgr_Callback
{
public:
    lodRead(TXPParser *in_parse) : _parse(in_parse)
    {};
    void *Parse(trpgToken tok,trpgReadBuffer &buf);
protected:
    TXPParser *_parse;
};

//----------------------------------------------------------------------------
class tileHeaderRead : public trpgr_Callback
{
public:
    tileHeaderRead(TXPParser *in_parse) : _parse(in_parse)
    {};
    void *Parse(trpgToken tok,trpgReadBuffer &buf);
protected:
    TXPParser *_parse;
};

//----------------------------------------------------------------------------

class modelRefRead : public trpgr_Callback
{
public:
    modelRefRead(TXPParser *in_parse) : _parse(in_parse)
    {};
    void *Parse(trpgToken tok,trpgReadBuffer &buf);
protected:
    TXPParser *_parse;
};

//----------------------------------------------------------------------------
class billboardRead : public trpgr_Callback
{
public:
    billboardRead(TXPParser *in_parse) : _parse(in_parse)
    {};
    void *Parse(trpgToken tok,trpgReadBuffer &buf);
protected:
    TXPParser *_parse;
};

//----------------------------------------------------------------------------
class lightRead: public trpgr_Callback
{
public:
    lightRead(TXPParser *in_parse) : _parse(in_parse)
    {};
    void *Parse(trpgToken tok,trpgReadBuffer &buf);
protected:
    TXPParser *_parse;
};

//----------------------------------------------------------------------------
class layerRead: public trpgr_Callback
{
public:
    layerRead(TXPParser *in_parse)  : _parse(in_parse)
    {};
    void *Parse(trpgToken tok,trpgReadBuffer &buf);
protected:
    TXPParser *_parse;
};

} // namespace

#endif // __TXPPARSER_H_

