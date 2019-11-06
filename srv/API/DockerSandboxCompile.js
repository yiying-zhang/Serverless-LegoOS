const fs = require("fs");
var unzipper = require("unzipper");

var DockerSandboxCompile = function (src_zip,
                     timeout_value,
                     path,
                     folder,
                     vm_name,
                     compiler_name,
                     file_name,
                     output_command,
                     languageName,
                     e_arguments)
{

    this.src_zip = src_zip;
    this.timeout_value=timeout_value;
    this.path=path;
    this.folder=folder;
    this.vm_name=vm_name;
    this.compiler_name=compiler_name;
    this.file_name=file_name;
    this.output_command=output_command;
    this.langName=languageName;
    this.extra_arguments=e_arguments;

}

DockerSandboxCompile.prototype.run = function (success) {

    var sandbox = this;
    this.prepare(() => sandbox.execute(success));
    //this.prepare(() => {});
}

DockerSandboxCompile.prototype.prepare = function (success) {
    var exec = require('child_process').exec;
    var fs = require('fs');
    var sandbox = this;
    var command = "mkdir "
        + this.path + this.folder
        + " && cp "+ this.path
        +"Payload/* "+this.path
        +this.folder+" && chmod 777 "
        + this.path+this.folder;

    console.log("execev command: " + command);

    exec(command, (st) => {

        var dst = require("path").join(sandbox.path, sandbox.folder);
        fs.createReadStream(sandbox.src_zip.path).pipe(unzipper.Extract({path: dst}));

        // put the main file into the workspace folder
        exec("chmod 777 \'"+sandbox.path+sandbox.folder+"/"+sandbox.file_name+"\'");
        success(); // callback, end of preparations, the success should be the execute function

        // fs.writeFile(sandbox.path + sandbox.folder+"/" + sandbox.file_name, sandbox.code, (err) => {
        //         if (err) {
        //             console.log(err);
        //         } else {
        //             console.log("execev command: " + ("chmod 777 \'"+sandbox.path+sandbox.folder+"/"+sandbox.file_name+"\'"));
        //
        //             exec("chmod 777 \'"+sandbox.path+sandbox.folder+"/"+sandbox.file_name+"\'");
        //             success(); // callback, end of preparations, the success should be the execute function
        //         }
        //     });
     });

}

DockerSandboxCompile.prototype.execute = function (success)
{
   var exec = require('child_process').exec;
   var fs = require('fs');
   var myC = 0; //variable to enforce the timeout_value
   var sandbox = this;

   //this statement is what is executed
   var st = this.path
       + 'DockerTimeout.sh '
       + this.timeout_value
       + 's -u mysql -e \'NODE_PATH=/usr/local/lib/node_modules\' -i -t -v  "'
       + this.path + this.folder
       + '":/usercode '
       + this.vm_name
       + ' /usercode/script.sh '
       + this.compiler_name
       + ' '
       + this.file_name + ' '
       + this.output_command+ ' '
       + this.extra_arguments;

   console.log(`start docker command: ${st}`);

   exec(st);
   console.log("------------------------------")
   //Check For File named "completed" after every 1 second
   var intid = setInterval(function() {
           //Displaying the checking message after 1 second interval, testing purposes only
           console.log("Checking " + sandbox.path+sandbox.folder + ": for completion: " + myC);

           myC = myC + 1;

       /**
        * the completed logs have been cached into completed folder;
        * the errors have beed saved to the errors;
        */
       fs.readFile(sandbox.path + sandbox.folder + '/completed', 'utf8', function(err, data) {

               // container not yet completed while not yet time out;
               if (err && myC < sandbox.timeout_value) {
                   console.log("file not found");
                   return;
               }
               // success
               else if (myC < sandbox.timeout_value) {
                   console.log("DONE")

                   //check for possible errors
                   fs.readFile(sandbox.path + sandbox.folder + '/errors', 'utf8', function(err2, data2) {

                       if(!data2) {
                           data2="";
                       }

                       console.log("Error file: ");
                       console.log(data2);

                       console.log("Main File");
                       console.log(data);

                       var lines = data.toString().split('*-COMPILEBOX::ENDOFOUTPUT-*');
                       data=lines[0];
                       var time=lines[1];

                       console.log("Time: ");
                       console.log(time);

                       success(data,time,data2); // done callback
                   });
               }
               //if time is up. Save an error message to the data variable
               else {
                   fs.readFile(sandbox.path + sandbox.folder + '/logfile.txt', 'utf8', function(err, data){
                       if (!data) {
                           data = "";
                       }
                       data += "\nExecution Timed Out";
                       console.log("Timed Out: "+sandbox.folder+" "+sandbox.langName)
                       fs.readFile(sandbox.path + sandbox.folder + '/errors', 'utf8', function(err2, data2) {
                            if(!data2) data2=""

                            var lines = data.toString().split('*---*')
                            data=lines[0]
                            var time=lines[1]

                            console.log("Time: ")
                            console.log(time)

                            success(data,data2)
                        });
                });

               }


               //now remove the temporary directory
               // console.log("ATTEMPTING TO REMOVE: " + sandbox.folder);
               // console.log("------------------------------")
               // exec("rm -r " + sandbox.folder);


               clearInterval(intid);
       });
   }, 1000);

}

module.exports = DockerSandboxCompile;
