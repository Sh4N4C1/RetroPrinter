# RetroPrinter

Based on [PrintSpoofer](https://github.com/itm4n/PrintSpoofer), but use custom RPC client which communicates with the spoolss pipe using raw data.

Just use NtWriteFile, NtReadFile, and NtFsControlFile winapi

<p align="center">
<img src="https://raw.githubusercontent.com/Sh4N4C1/gitbook/main/images/RetroPrinter.png" alt="RetroPrinter">
</p>

## usage

```
make # on linux

.\RetroPrinter.exe C:\Windows\System32\cmd.exe
```

if you want more output

```
make debug
```

## Credits

https://www.x86matthew.com/view_post?id=create_svc_rpc

https://github.com/itm4n/PrintSpoofer
