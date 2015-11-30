How to make:
	On x86/x64 Linux platform: # make linux
	On 3560E ARM Linux platform: # make 3560E

How to run:
# ./fv_httpd -auth_realm fonsview.com -ports 80 -root ./web 

How to enable http digest auth:
# ./fv_httpd -A ./web/.htpasswd fonsview.com username password
web/.htpasswd file is a text file with username/encypted password, delete a user can be simply done by delete the line contain the user's username.

