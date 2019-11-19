var express = require('express');
var http = require('http');
var arr = require('./compilers');
var sandBox = require('./DockerSandboxCompile');
var bodyParser = require('body-parser');
const formidable = require('formidable');

var app = express();

var port=8080;

var ExpressBrute = require('express-brute');
var store = new ExpressBrute.MemoryStore(); // stores state locally, don't use this in production
var bruteforce = new ExpressBrute(store,{
    freeRetries: 50,
    lifetime: 3600
});

var random = (size) => require("crypto").randomBytes(size).toString('hex');

app.use((req, res, next) => {
    next();
});

function file_parser_async (req) {
    var msg = {};
    // async, await only works for the promises
    return new Promise((resolve, reject) => {
        new formidable.IncomingForm().parse(req)
        .on('file', (name, file) => {
            msg[name] = file;
        })
        .on('aborted', () => reject({err: 'abort'}))
        .on('error', (err) => {
            reject({err: err})
        })
        .on('end', () => {
            resolve(msg);
        })
})}

// for handle the compiled code
async function cb (req, res) {

    try {
        var msg = await file_parser_async(req);
        console.log("messgae: ", msg);

        file = msg['file'];

        if (!file) {
            res.status(404).json({ok: false, errors: "function file missing"});
            return;
        }

        var folder = 'temp/' + random(10);
        var path = __dirname+"/";
        var vm_name = 'virtual_machine';
        var timeout_value = 20; // in seconds
        var language = 5; //  compiler for the python

        var sandboxType = new sandBox(
            file,
            timeout_value,
            path,
            folder,
            vm_name,
            arr.compilerArray[language][0],
            arr.compilerArray[language][1],  // file.XX, the main function file
            arr.compilerArray[language][2],  // output command
            arr.compilerArray[language][3],  // language name
            arr.compilerArray[language][4]); // extra_argument

        console.log("this scope check: ", sandboxType.timeout_value); // 20
        console.log("this scope check: ", this.timeout_value); // undefined

        sandboxType.run((data, exec_time, err) => {
            res.json({output:data, langid: 0, errors:err, time:exec_time});
        });

        // sandboxType.prepare(() => {});

    } catch (err) {
        console.log(err);
        res.status(404).json({ok: false, errors: err});
    }


}

app.put('/compile', cb);

app.get('/', (req, res)=> {
    res.status(404).json({ok: false, message: "not implemented", client: req.connection.remoteAddress});
});

app.get('/index.html', (req, res)=> {
    res.sendFile("./index.html", {root: __dirname});
});

app.listen(port, () => {
    console.log("server start, listening at "+port);
});
