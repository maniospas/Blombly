!modify "sftp://"

// create new server instance for 1 hour at: https://sftpcloud.io/tools/free-ftp-server
username = "d4f6e29c96eb48629cee8fce086181ff";
password = "lTKsr63ApK355ZJjMwlRkSq4OoJtjLl1";

// create file from ftp
ftp = "sftp://eu-central-1.sftpcloud.io/test_file"|file;
ftp["username"] = username;
ftp["password"] = password; 
ftp["timeout"] = 10;
push(ftp, "Pushed test file contents.");

// retrieve file from ftp
ftp = "sftp://eu-central-1.sftpcloud.io/test_file"|file;
ftp["username"] = username;
ftp["password"] = password; 
ftp["timeout"] = 10;
ftp |= bb.string.join("\n");
print(ftp);