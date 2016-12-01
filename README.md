# OS-hw3
## summary
-  author:丁國騰(Kuo Teng, Ding)
-  student_id:E94036209
-  class:資工三乙
-  title:hw3

##  Usage:
-  makefile:

```
  make
```

-  open server:

```
./server \[-s server\] \[-p port\] \[-h\]
  -s server: specify the server name or address, default: 127.0.0.1
  -p port: specify the server port, default: 12345
  -h :for print this help message
```

- open client:

```
./client \[-u\] \[-r run\] \[-s server\] \[-p port\] \[-h\]
  -u: create request by user input, default: random generation
  -r run: how many request to send
  -s server: specify the server name or address, default: 127.0.0.1
  -p port: specify the server port, default: 12345
  -h :for print this help message
```

    - as same as the usage of the original code assistant gave

## description&programming logic:
### server.c:
#### main function:
    我使用和助教一樣的getopt函式來擷取指令，並將port和domain預設為12345及127.0.0.1
    再以getaddrinfo獲取addrinfo鍊結串列s，並嘗試以其中的資訊打開socket
    最後設置socket、bind且listen它
    而在main function的最後，以一個while無窮迴圈重複查看是否有新的accept進來，若有則create一個新的pthread處理
    而while迴圈結束後，close剛剛開啟的socket

#### server_thread:
    處理特定accept進來的client，並且以while迴圈以read不斷抓取新的訊息，並write來傳輸回應
    每次先比對request字串，如果不是SET、GET、INFO其中一者，則傳回405訊息
    而每次都將domain name字串全部轉為小寫字母，以處理大小寫判別問題
    又如果domain name中沒有找到任何'.'符號或是其在第一個位置，則代表domain name錯誤
    每次while循環，將會以memset清除buf和buf_len之值
