/*! \file c4s-build.cpp
 * \brief Source code for the DirectDatabase library build.
 * 0.3: better output during install what is library is being copied.
 */
// Copyright (c) Menacon Oy
/********************************************************************************/
#include <cpp4scripts.hpp>
using namespace c4s;

program_arguments args;

const char *files_common = "directdatabase.cpp ddbrowset.cpp ddbpostgre.cpp ddbpostgrers.cpp"; // ddbmysql.cpp ddbmysqlrs.cpp";
const char *files_win    = "ddbodbc.cpp ddbodbcrs.cpp";

// ============== LINUX ===============================================================================
#if defined(__linux) || defined(__APPLE__)
int Build(bool wxmode)
{
    path_list cppFiles(files_common, ' ');

    int flags = BUILD_LIB|BUILD_PAD_NAME;
    flags |= args.is_set("-deb") ? BUILD_DEBUG : BUILD_RELEASE;
    if(args.is_set("-V"))
        flags |= BUILD_VERBOSE;
    const char *subsys =  args.is_value("-t","WX") ? "wx":"stl";
    builder_gcc make(&cppFiles,"directdb",&cout,flags,subsys);
    make.include_variables();
    make.add_comp("-Wno-ctor-dtor-privacy -Wnon-virtual-dtor -I$(LIBPQ)/interfaces/libpq -I$(LIBPQ)/include -I$(C4S)/include/cpp4scripts");
    //make.add_comp("-I/usr/include/mysql");
    if(args.is_set("-wx")) {
        make.add_comp("-DDDB_USEWX");
    }
    else {
        make.add_comp("-fno-rtti -DDDB_USESTL");
    }
    if(args.is_set("-deb"))
        make.add_comp("-DC4S_LOG_LEVEL=2");
    else
        make.add_comp("-DC4S_LOG_LEVEL=5");
    return make.build();
}

// ============= MS-WINDOWS ===========================================================================
#else
int Build(bool wxmode)
{
    path_list cppFiles(files_common, ' ');
    //cppFiles.add(files_win,' ');

    const char *subsys =  wxmode ? "wx":"stl";
    const char *wxopts = wxmode ? "":0;

    int flags = BUILD_LIB|BUILD_PAD_NAME;
    flags |= args.is_set("-deb") ? BUILD_DEBUG : BUILD_RELEASE;
    if(args.is_set("-V"))
        flags |= BUILD_VERBOSE;
    if(wxmode)
        flags |= BUILD_WIDECH|BUILD_NODEFARGS;
    builder_vc make(&cppFiles,"directdb",&cout,flags,subsys);
    make.include_variables();
    make.include_variables("c:\\Devtools\\wxvariables-2.9.txt");
    make.add_comp("/DUSE_SSL /I$(BINC)\\cpp4scripts /I$(BINC)\\libpq"); // /I$(LIBPQ)\\include
    if( args.is_set("-deb") ) {
        make.add_comp("/DC4S_LOG_LEVEL=2");
        if(!wxmode)
            make.add_comp("/D_DEBUG");
    }
    else
        make.add_comp("/DC4S_LOG_LEVEL=4");
    if(wxmode) {
        make.add_comp("/c /DDDB_USEWX");
        if( args.is_set("-deb") )
            make.add_comp(make.get_arch()==BUILD_X64?"$(WX_COMP64_DEB)":"$(WX_COMP32_DEB)");
        else
            make.add_comp(make.get_arch()==BUILD_X64?"$(WX_COMP64_REL)":"$(WX_COMP32_REL)");
        if( make.precompile("wx","#include <wx/wxprec.h>\n#include \"pch-stop.h\"", "pch-stop.h") ) {
            cerr << "Precompilation failed.\n";
            return 1;
        }
    }
    else
        make.add_comp("/DDDB_USESTL");

    return make.build();
}
#endif

// ====================================================================================================
int Clean(bool wxmode)
{
    const char *subsys = wxmode ? "wx":"stl";
    try{
#if defined(__linux) || defined(__APPLE__)
        builder_gcc(0,"",0,BUILD_LIB|BUILD_DEBUG|BUILD_PAD_NAME, subsys).clean_build_dir();
        builder_gcc(0,"",0,BUILD_LIB|BUILD_RELEASE|BUILD_PAD_NAME, subsys).clean_build_dir();
#else
        builder_vc(0,"",0,BUILD_DEBUG|BUILD_PAD_NAME, subsys).clean_build_dir();
        builder_vc(0,"",0,BUILD_RELEASE|BUILD_PAD_NAME, subsys).clean_build_dir();
#endif
    }
    catch(path_exception pe){
        cerr << pe.what() << endl;
        return 1;
    }
    cout << "Build directories cleaned!\n";
    return 0;
}
// ====================================================================================================
int install(bool wxmode)
{
    const char *subsys = wxmode ? "wx":"stl";
    const char *name="directdb";
#if defined(__linux) || defined(__APPLE__)
    path lib_d = builder_gcc(0,name,0,BUILD_LIB|BUILD_PAD_NAME|BUILD_DEBUG, subsys).get_target_path();
    path lib_r = builder_gcc(0,name,0,BUILD_LIB|BUILD_PAD_NAME|BUILD_RELEASE, subsys).get_target_path();
#else
    path lib_d = builder_vc(0,name,0,BUILD_LIB|BUILD_PAD_NAME|BUILD_DEBUG, subsys).get_target_path();
    path lib_r = builder_vc(0,name,0,BUILD_LIB|BUILD_PAD_NAME|BUILD_RELEASE, subsys).get_target_path();
#endif

    try {
        string root = append_slash(args.get_value("-install"));
        if (lib_d.exists()) {
            cout << "Copying "<<lib_d.get_path()<<'\n';
            lib_d.cp(path(root+"lib/"),PCF_FORCE);
        }
        if (lib_r.exists()) {
            cout << "Copying "<<lib_r.get_path()<<'\n';
            lib_r.cp(path(root+"lib/"),PCF_FORCE);
        }

        path_list ipl(path("./"),"*.hpp");
        ipl.copy_to(path(root+"include/directdb/"),PCF_FORCE);
    }catch(c4s_exception ce) {
        cout << "Install failed: "<<ce.what()<<'\n';
        return 1;
    }
    return 0;
}
// ====================================================================================================
int main(int argc, char **argv)
{
    int rv=1;
    string mode, binding;
    bool wxmode=false;

    // Parameters
    args += argument("-deb",  false, "Sets the debug mode");
    args += argument("-rel",  false, "Sets the release mode");
    args += argument("-t",    true,  "Set the build type [WX|STL]");
    args += argument("-V",    false, "Enable verbose build mode");
    args += argument("-install", true, "Install library to given root.");
    args += argument("-clean",false, "Clean up build files.");

    cout << "Direct Database library build v 0.3\n";
    try{
        args.initialize(argc,argv,1);
    }
    catch(c4s_exception ce){
        cerr << "Error: " << ce.what() << endl;
        args.usage();
        return 1;
    }

    if(args.is_value("-t","STL")) {
        wxmode = false;
    } else if(args.is_value("-t","WX")){
        wxmode = true;
    }
    else {
        cout << "Missing target (WX or STL).\n";
        return 1;
    }
    if(args.is_set("-clean"))
        return Clean(wxmode);

    if(args.is_set("-install"))
        return install(wxmode);

    try {
        rv = Build(wxmode);
    } catch(c4s_exception ce) {
        cout << "Build failed: "<<ce.what()<<'\n';
    }
    cout << "Build finished.\n";
    return rv;
}
