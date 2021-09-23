//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#if defined(ZIP_SUPPORT)

#ifndef FS_NODE_ZIP_HXX
#define FS_NODE_ZIP_HXX

#include "ZipHandler.hxx"
#include "FSNode.hxx"

/*
 * Implementation of the Stella file system API based on ZIP archives.
 * ZIP archives are treated as directories if the contain more than one ROM
 * file, as a file if they contain a single ROM file, and as neither if the
 * archive is empty.  Hence, if a ZIP archive isn't a directory *or* a file,
 * it is invalid.
 *
 * Parts of this class are documented in the base interface class, AbstractFSNode.
 */
class FilesystemNodeZIP : public AbstractFSNode
{
  public:
    /**
     * Creates a FilesystemNodeZIP for a given path.
     *
     * @param path  String with the path the new node should point to.
     */
    explicit FilesystemNodeZIP(const string& path);

    bool exists() const override;
    const string& getName() const override    { return _name; }
    void setName(const string& name) override { _name = name; }
    const string& getPath() const override { return _path;      }
    string getShortPath() const   override { return _shortPath; }
    bool hasParent() const override   { return true; }
    bool isDirectory() const override { return _isDirectory; }
    bool isFile() const      override { return _isFile;      }
    bool isReadable() const  override { return _realNode && _realNode->isReadable(); }
    bool isWritable() const  override { return false; }

    //////////////////////////////////////////////////////////
    // For now, ZIP files cannot be modified in any way
    bool makeDir() override { return false; }
    bool rename(const string& newfile) override { return false; }
    //////////////////////////////////////////////////////////

    bool getChildren(AbstractFSList& list, ListMode mode) const override;
    AbstractFSNodePtr getParent() const override;

    size_t read(ByteBuffer& image) const override;
    size_t read(stringstream& image) const override;
    size_t write(const ByteBuffer& buffer, size_t size) const override;
    size_t write(const stringstream& buffer) const override;

  private:
    FilesystemNodeZIP(const string& zipfile, const string& virtualpath,
        const AbstractFSNodePtr& realnode, bool isdir);

    void setFlags(const string& zipfile, const string& virtualpath,
        const AbstractFSNodePtr& realnode);

    friend ostream& operator<<(ostream& os, const FilesystemNodeZIP& node)
    {
      os << "_zipFile:     " << node._zipFile << endl
         << "_virtualPath: " << node._virtualPath << endl
         << "_path:        " << node._path << endl
         << "_shortPath:   " << node._shortPath << endl;
      return os;
    }

  private:
    /* Error types */
    enum class zip_error
    {
      NONE,
      NOT_A_FILE,
      NOT_READABLE,
      NO_ROMS
    };

    // Since a ZIP file is itself an abstraction, it still needs access to
    // an actual concrete filesystem node
    AbstractFSNodePtr _realNode;

    string _zipFile, _virtualPath;
    string _name, _path, _shortPath;
    zip_error _error{zip_error::NONE};
    uInt32 _numFiles{0};

    bool _isDirectory{false}, _isFile{false};

    // ZipHandler static reference variable responsible for accessing ZIP files
    static unique_ptr<ZipHandler> myZipHandler;
};

#endif

#endif  // ZIP_SUPPORT
