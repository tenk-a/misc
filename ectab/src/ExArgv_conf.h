// ---------------------------------------------------------------------------
// Configuration settings.

// [] Define to generate ExArgv_forWinMain for use with WinMain.
//#define EXARGV_FOR_WINMAIN

// [] Define one of the following for path string character encoding.
//    If omitted, WCHAR is used if UNICODE is defined, otherwise UTF8.
//    Assumes UTF8 for non-Windows. MBC is for considering SJIS, etc.
//#define EXARGV_ENCODE_ANSI    // Use A-type API on Windows. Assumes UTF8 for non-Windows.
//#define EXARGV_ENCODE_MBC     // Use A-type API on Windows. Considers 2nd byte of DBC.
//#define EXARGV_ENCODE_UTF8    // Use W-type API on Windows for UTF8 encoding. Same as ANSI for non-Windows.
//#define EXARGV_ENCODE_WCHAR   // Use W-type API on Windows. Not for non-Windows.

// [] Enable wildcard specification: 1=enabled, 0=disabled, undefined=1
//#define EXARGV_USE_WC             1
#define EXARGV_USE_WC               1

// [] For wildcard enabled, recursive search if wildcard ** is present:
//      1=enabled, 0=disabled, undefined=1
//#define EXARGV_USE_WC_REC         1
#define EXARGV_USE_WC_REC           1

// [] Enable @response files:
//      1=enabled, 0=disabled, undefined=0
//#define EXARGV_USE_RESFILE        0
#define EXARGV_USE_RESFILE          1

// [] Enable simple config (response) file input:
//      1=enabled, 0=disabled, undefined=0
//    When enabled, replaces .exe with .ini for win/dos.
//    For non-Windows, assumes unix-style ~/.executable_name.ini
//#define EXARGV_USE_CONFIG         0

// [] When config file input is enabled, define this to set the config file extension.
//#define EXARGV_CONFIG_EXT         ".ini"

// [] Define to use this environment variable as the command line string.
//#define EXARGV_ENVNAME            "your_app_env_name"

// [] Windows only. Full path for argv[0] executable name:
//      1=enabled, 0=disabled, undefined=0
//    Note: bcc, dmc, watcom already use full path, so this is for vc, gcc.
//#define EXARGV_USE_FULLPATH_ARGV0  0

// [] Define to replace \ with / in filepath.
//#define EXARGV_TOSLASH

// [] Define to replace / with \ in filepath.
//#define EXARGV_TOBACKSLASH

// [] Treat / as option start character (1) or not (0)
//#define EXARGV_USE_SLASH_OPT      0


// ---------------------------------------------------------------------------
