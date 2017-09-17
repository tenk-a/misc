/**
 *  @file   ExArgv.h
 *  @brief  main(int argc,char* argv[]) のargc,argvを、
 *  	    レスポンスファイル、ワイルドカード展開したものに拡張する.
 *  @author Masashi KITAMURA
 *  @date   2007
 */

#ifndef EXARGV_H
#define EXARGV_H


#if defined __cplusplus == 0

/// argc,argv をレスポンスファイルやワイルドカード展開して、argc, argvを更新して返す.
void ExArgv_get(int* pArgc, char*** pppArgv, const char* flags, const char* envName, const char* recOpt1, const char* recOpt2);

    /*	[flags文字]
     *	    @	    @response_file 有り.
     *	    C	    .exeを.cfgに変えたファイルパス名のコンフィグを読み込む.
     *
     *	    *	    wildcard 文字有り.
     *	    r	    ワイルドカード指定のみ、ディレクトリ再帰検索する.
     *	    R	    ファイル名は全てディレクトリ再帰検索する.
     *	    i	    再帰オプション文字列の大文字小文字を判別しない.
     *	    --	    再帰オプション検索時,--以降、-で始まる文字列はオプションでなくファイル名.
     *
     *	※ マクロ EXARGV_TINY 定義時は @cおよび環境変数のみ有効。*rRi--は不可.
     */


#ifdef _WINDOWS
/// winアプリで、WinMain初っ端で、argc,argvを作る場合用.
void ExArgv_forWinMain(const char* pCmdLine, int* pArgc, char*** pppArgv, const char* flags, const char* envName, const char* recOpt1, const char* recOpt2 );
#endif	// _WINDOWS



#else	// c++ のときは、デフォルト引数を設定.
extern "C" {
    void ExArgv_get(int* pArgc, char*** pppArgv, const char* flags="@*", const char* envName=0, const char* recOpt1=0, const char* recOpt2=0);
   #ifdef _WINDOWS
    void ExArgv_forWinMain(const char* pCmdLine, int* pArgc, char*** pppArgv, const char* flags="@*", const char* envName=0, const char* recOpt1=0, const char* recOpt2=0 );
   #endif
}
#endif


#endif	// EXARGV_H
