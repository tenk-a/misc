rem 16ビットMSDOS環境用
bcc -1 -ml -Ox -DDOS16 -eMWFND_16.EXE mwfnd.c tree.c noehl.lib
