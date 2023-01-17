# Directory-Manager
A directory management system that allows a company to separate employee access to information. Employees may be assigned to a directory and cannot access different directories’ information. 

## Administrator Application

The administrator application allows admins to control the server, view users activity, and create repositories. Admins may create other administrator accounts as well. When a repository is created, and selected from the drop down menu, they may add users to that repository, and set their permission level.

To set up the Administrator Applicaiton, you must do the following:

1. Update the PostgreSQL connection parameters at the beginning of these files:

      A. server.cpp
  
      B. clientworker.cpp
  
      C. mainwindow.cpp
     
      ![image](https://user-images.githubusercontent.com/44332803/212836796-ccd57bbd-9313-47d5-8bc8-4ebb3b89599e.png)

  
2. Update the directory path where the repositories are stored in the following files:

      A. server.cpp
  
      B. clientworker.cpp
      
      ![image](https://user-images.githubusercontent.com/44332803/212836967-04283071-def5-4292-afe7-d3fdb44ec073.png)

3. Upon startup, the application prompts the admin to sign in. You may use the following SQL query to add a starter admin account:
    
            INSERT INTO public."Users"("ID", "firstName", "lastName", "Email", "Password", "isAdmin") 
                  VALUES (0, 'Admin', 'Admin', 'Admin', 'Admin', 1);
      
      It is recommended to create an admin account with stronger credentials immediately. Use the following to delete the old admin account:
      
            DELETE FROM public."Users" WHERE "ID" = 0;
      
      

## Client Application

To set up the Client Applicaiton, you must do the following:

1. Update the connection parameters at the beginning of these files:

      A. client.cpp (PostgreSQL Setup)
      
      ![image](https://user-images.githubusercontent.com/44332803/212840584-e0a135f7-d638-438c-b643-4d689292f4fa.png)
      
      B. client.cpp (Server Connection)
      
      ![image](https://user-images.githubusercontent.com/44332803/212840778-ebd4d6d2-ba18-4da8-baac-7da6b493db08.png)

  
2. Update the directory path where the repositories are stored in the following files:

      A. clientapplication.cpp
        
      ![image](https://user-images.githubusercontent.com/44332803/212841142-7c3fb309-1d5f-40e4-adc2-423a48e30e10.png)




Note: The port number for the sever/client connection is 5555. This can be changed in the files.
