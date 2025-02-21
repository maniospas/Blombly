!modify "sftp://"

// create new server instance for 1 hour at: https://sftpcloud.io/tools/free-ftp-server
username = "d665ea640ee249bab87431159d231c98";
password = "YK901g7ppwn2LPktuTitIJxGWqMezSwx";

// create file from ftp
ftp = "sftp://eu-central-1.sftpcloud.io/test_file"|file;
ftp.io::username = username;
ftp.io::password = password; 
ftp.io::timeout = 10;
push(ftp, "These are the test file contents that are pushed and are to be retrieved.");

// retrieve file from ftp
ftp = "sftp://eu-central-1.sftpcloud.io/test_file"|file;
ftp.io::username = username;
ftp.io::password = password; 
ftp.io::timeout = 10;
ftp |= bb.string.join("\n");
print(ftp);