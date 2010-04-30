#ifndef SDKFILE_HPP_
#define SDKFILE_HPP_

#include <vector>
#include <string>
#include <fstream>

namespace streamsdk
{
//! Get current directory
extern std::string getCurrentDir();

//! \class SDKFile for the opencl program processing
class SDKFile
{
public:
    //! Default constructor
    SDKFile(): source_("") {}

    //! Destructor
    ~SDKFile();

    //! Opens the CL program file
    bool open(const char* fileName);

    //! Returns a pointer to the string object with the source code
    const std::string&  source() const { return source_; }

private:
    //! Disable copy constructor
    SDKFile(const SDKFile&);

    //! Disable operator=
    SDKFile& operator=(const SDKFile&);

    std::string     source_;    //!< source code of the CL program
};

} // namespace streamsdk

#endif  // SDKFile_HPP_
