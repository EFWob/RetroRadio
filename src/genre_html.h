// genre.html file in raw data format for PROGMEM
//
#define genre_html_version 1210514
const char genre_html[] PROGMEM = R"=====(<!DOCTYPE html>
<html>
 <head>
  <title>ESP32-radio</title>
  <meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
  <link rel="stylesheet" type="text/css" href="radio.css">
  <link rel="Shortcut Icon" type="image/ico" href="favicon.ico">
 </head>
 <body>
  <ul>
   <li><a class="pull-left" href="#">ESP32 Radio</a></li>
   <li><a class="pull-left" href="/index.html">Control</a></li>
   <li><a class="pull-left active" href="/genre.html">Genre</a></li>
   <li><a class="pull-left" href="/config.html">Config</a></li>
   <li><a class="pull-left" href="/mp3play.html">MP3 player</a></li>
   <li><a class="pull-left" href="/about.html">About</a></li>
  </ul>
  <br><br><br>
  <center>
   <h1>** ESP32 Radio Genre Manager **</h1>
   <table id="genreDir" width="450" class="pull-left">
   <tr>
    <td>Loading...</td>
   </tr>
   </table>
   <div id="apply">
   </div>
   <table class="table2" id="stationsTable" width="450">
   </table>
  </center>
 </body>
</html>

<script>

 var dirArr = [];
 var actionArray = [];
 var stationArr = [];

 var validGenres = 0;
 var validPresets = 0;
 var xhr ;
 var requestId = 0; 
  function getAction ( actionId, actions )
  {
//    var actions = ["none", "refresh", "delete"];
    var select = document.createElement("select");
    select.name = actionId;
    select.id = actionId;
    for (const val of actions)
        {
            var option = document.createElement("option");
            option.value = val;
            option.text = val.charAt(0).toUpperCase() + val.slice(1);
            select.appendChild(option);
        }
    return select;
  }

 function listDir ( )
 {
  var table = document.getElementById('genreDir') ;
  table.innerHTML = "Please wait for the list of stored genres to load...";
  var theUrl = "genredir";//?version=" + Math.random() ;
  xhr = new XMLHttpRequest() ;
  xhr.onreadystatechange = function() 
  {
    if ( this.readyState == XMLHttpRequest.DONE )
    {
      var table = document.getElementById('genreDir') ;
      table.innerHTML = "" ;
      validGenres = 0;
      validPresets = 0;
      var row = table.insertRow() ;
      var cell1 = row.insertCell(0) ;
      var cell2 = row.insertCell(1) ;
      var cell3 = row.insertCell(2) ;
      cell1.innerHTML = "Genre" ;
      cell2.innerHTML = "Presets" ;
      cell3.innerHTML = "Action" ;
      dirArr = JSON.parse ( this.responseText ) ;
      var i ;
      dirArr = dirArr.sort(function(a, b) {
            return b.name < a.name? 1
                   : b.name > a.name? -1
                   :0
      });
      for ( i = 0 ; i <= dirArr.length ; i++ )
      {
        if ((i == dirArr.length) && (validGenres > 0))
          {
            var row = table.insertRow() ;
            var cell1 = row.insertCell(0) ;
            var cell2 = row.insertCell(1) ;
            var cell3 = row.insertCell(2) ;
            cell1.innerHTML = "Total:";
            cell2.innerHTML = validPresets ;
            var select = getAction("totaldir", ["None", "Refresh"]);
            select.onchange=function() 
            {
                var i;
                for (i = 0;i < dirArr.length;i++)
                {
                    var selector = document.getElementById("action" + i);
                    if (selector != null)
                    {
                      selector.value = this.value;
                      dirArr[i].action = this.value;
                    }
                }
            }
            cell3.appendChild(select);
            row = table.insertRow();
            cell1 = row.insertCell(0) ;
            cell1.colSpan = 3 ;
            cell1.innerHTML = 
             'Press  <button class="button" onclick="runActions(dirArr)">HERE</button> to perform the Actions!' ;

          }  
        else if (dirArr[i].mode == "<valid>")
          {
            var row = table.insertRow() ;
            var cell1 = row.insertCell(0) ;
            var cell2 = row.insertCell(1) ;
            var cell3 = row.insertCell(2) ;
            cell1.innerHTML = dirArr[i].id + ": " + dirArr[i].name;
            cell2.innerHTML = dirArr[i].presets ;
            validGenres++ ;
            validPresets = validPresets + dirArr[i].presets ;
            var select=getAction("action"+i, ["None", "Refresh", "Delete"]);
            dirArr[i].action = "None" ;
            select.onchange=function() 
            {
                var idx = this.id.substring(6);
 //               alert("Change for " + idx + " to: " + this.value);
                dirArr[idx].action = this.value;
            }

/*            
            var select = document.createElement("select");
            var actions = ["none", "refresh", "delete"];
            select.name = "action" + i;
            select.id = "action" + i;
            for (const val of actions)
              {
                var option = document.createElement("option");
                option.value = val;
                option.text = val.charAt(0).toUpperCase() + val.slice(1);
                select.appendChild(option);
              }
*/
            cell3.appendChild(select);  
          }
      }
    }
  }
  xhr.open ( "GET", theUrl ) ;
  xhr.send() ;
 }





 var resultContainer = null;
 var resultTimeout = null; 
 function actionDone(id, result)
 {
     clearTimeout ( resultTimeout );resultTimeout = null;
     if (result.substring(0, 2) == "OK")
     {
        if (resultContainer != null)
          if (result.substring(2,3) == "=")
            resultContainer.innerHTML = result.substring(3) ;
          else
            resultContainer.innerHTML = "OK";
         runAction(id + 1);
     }
     else
     {
        if (resultContainer != null)
            resultContainer.innerHTML = "ERROR!" ;
         alert("Action for " + id + " failed with result: " + result + ". Action run will be stopped!");
         runAction(dirArr.length);
     }
 }   

 function showAction (description)
 {
    var table = document.getElementById('genreDir') ;
    var row = table.insertRow() ;
    var cell1 = row.insertCell(0) ;
    var cell2 = row.insertCell(1) ;
    cell1.innerHTML = description;
    resultContainer = cell2;
    cell2.innerHTML = "..busy..";
 }
 
 function uploadStatChunk(id)
 {
   resultContainer.innerHTML = actionArray[id].subIdx + " presets";
   if (actionArray[id].subIdx >= stationArr.length)
   {
     actionDone(id, "OK=" + actionArray[id].subIdx + " presets");
   }
   else
   {
     var delta = 20;
     var stations = "";
     var l = stationArr.length - actionArray[id].subIdx;
     if (l > delta)
      l = delta;
     for(i = 0;i < l;i++)
      stations=stations + "%7c" + stationArr[i + actionArray[id].subIdx].url_resolved; 
     runActionRequest(id, "save=" + actionArray[id].id + 
                          ",genre=" + actionArray[id].name +
                          ",count=" + l +
                          ",idx=" + actionArray[id].subIdx +
                          stations,
                          3000, uploadStatChunkCB);
    actionArray[id].subIdx += l;
   }
 }

 function uploadStatChunkCB (id, result)
 {
   clearTimeout ( resultTimeout );resultTimeout = null;
   if (result.substr(0,2) == "OK")
   {
    uploadStatChunk(id);
   }
   else
   {
     actionDone (id, "Upload failed, Radio returned: " + result ) ;
   }
 }

 function listStatsDelCB ( id, result)
 {
   if (result.substring(0, 2) == "OK")
   {
      showAction ("Uploading " + stationArr.length + " presets to radio.");
      resultContainer.innerHTML = "0 presets";
      actionArray[id].subIdx = 0;
      uploadStatChunk(id);
    }
    else
      actionDone(id, "Radio reports error on deleting genre " + actionArray[id].name);
 }

 function listStatsCB ( id, genre, loaded)
 {
    clearTimeout ( resultTimeout );resultTimeout = null;
    if (loaded && (stationArr.length > 0))
    {
      resultContainer.innerHTML = stationArr.length + " presets";
/*
      showAction(`First deleting genre '${genre}'`); 
      runActionRequest( id, "del=" + actionArray[id].id + ",genre=" + 
              actionArray[id].name, 0, listStatsDelCB) ;
*/              
      showAction ("Uploading " + stationArr.length + " presets to radio.");
      resultContainer.innerHTML = "0 presets";
      actionArray[id].subIdx = 0;
      uploadStatChunk(id);
    } 
    else if (loaded)
    {
      actionDone(id, "RDBS returned 0 presets for genre '" + genre + "'.");
    }
    else
    {
      actionDone(id, "Connect to RDBS timed out.");
    }
 }

function runActionRequest (id, theUrl, timeout, callback)
{   
  xhr = new XMLHttpRequest() ;
  //xhr.setRequestHeader("Cache-Control", "no-cache, no-store, max-age=0");
 // xhr.setRequestHeader("Cache-Control",  "no-cache, no-store, must-revalidate");
  theUrl="genreaction?" + theUrl + "&version=" + Math.random();

  xhr.onreadystatechange = function() 

  {
    if ( this.readyState == XMLHttpRequest.DONE )
    {
      callback ( id, this.responseText.trim() ) ;
    }
  }  
  clearTimeout(resultTimeout);resultTimeout = null;
  if (timeout)
    resultTimeout = setTimeout(actionDone, timeout, id, `Timeout for URL: ${theUrl}`);
  xhr.open ( "GET", theUrl ) ;
  xhr.send() ;
  
}


 function doDelete( idx )
 {
     resultTimeout = setTimeout( actionDone, 2000, idx, "OK: Delete but Timeout!") ;
 }

 function runAction( idx )
 {
//      alert(idx + ":" + actionArray[idx].action + "(" + actionArray[idx].name + ")");

     clearTimeout (resultTimeout );
     resultTimeout = null ;
     if (idx < actionArray.length)
        while ((idx < actionArray.length) && 
          ((actionArray[idx].action == "None") || (actionArray[idx].action == undefined)))
            idx++;
     if ( idx >= actionArray.length)
     {
        location.reload();
     }
     else
     {
         //alert(idx + ":" + actionArray[idx].action + "(" + actionArray[idx].name + ")");
         if ( actionArray[idx].action == "Delete")
         {
            //listDir();
            //alert("Just running listDir....");
            //alert(`Deleting genre '${actionArray[idx].name}'...`);
            showAction(`Deleting genre '${actionArray[idx].name}'`); 
            runActionRequest( idx, "del=" + actionArray[idx].id + ",genre=" + 
              actionArray[idx].name, 0, actionDone) ;
         }
         else if ( actionArray[idx].action == "Refresh")
         {
            showAction(`Reload genre '${actionArray[idx].name}'`);
            listStats(idx, actionArray[idx].name, 5000, listStatsCB) ;
         }
         else
         {
            resultContainer = null;
            alert(`Unexpected: unknown action "${actionArray[idx].action}" for genre '${actionArray[idx].name}'`);
            actionDone(idx, "This is not nice!") ;
        }
     }
 }

 function runActions(newActions)
 {
    if (actionArray.length > 0)
    {
      alert("There are already ongoing actions. Please wait and retry!");
      return;
    }  
    if (newActions.length == 0)
    {
      alert("There is nothing to do!");
      return;
    }  
    actionArray = newActions;
    alert("ActionArray length= " + actionArray.length);
    var table = document.getElementById('genreDir') ;
    table.innerHTML = "" ;
    var row = table.insertRow() ;
    var cell1 = row.insertCell(0) ;
    cell1.colSpan = 2;
    cell1.innerHTML = "Please wait for page to reload. Do not load any other page now!"; 
    runAction (0) ;
/*
    for (i = 0; i < dirArr.length; i++)
      if (dirArr[i].mode != "None")
        {
        var action = dirArr[i].action;    
        var genre = dirArr[i].name;
        var id = dirArr[i].id;
        if ((action == "Refresh") || (action == "Delete"))
            deleteGenre ( genre, id ) ;
        }
    location.reload();      
*/    
 }



 // Get info from a radiobrowser.  Working servers are:
 // https://de1.api.radio-browser.info, https://fr1.api.radio-browser.info, https://nl1.api.radio-browser.info
 // id: reference to be passed on to callback
 // genre: genre to load
 // timeout: in ms for response from RDBS
 // callback: to be called on either timeout (empty stationArr) or success (filled stationArr)
 function listStats ( id, genre, timeout, callback )
 {
  var theUrl = "https://nl1.api.radio-browser.info/json/stations/bytag/" +
               genre +
               "?hidebroken=true" ;
  xhr = new XMLHttpRequest() ;
  stationArr = [];
  xhr.onreadystatechange = function() 
  {
   if ( this.readyState == XMLHttpRequest.DONE )
   {
    //var table = document.getElementById('stationsTable') ;
    //table.innerHTML = "" ;
    stationArr = JSON.parse ( this.responseText ) ;
    var i ;
    var snam ;
    var oldsnam = "" ;
    for ( i = 0 ; i < stationArr.length ; i++ )
    {
      snam = stationArr[i].name ;
      if ( stationArr[i].url_resolved.startsWith ( "http:") &&            // https: not supported yet
           snam != oldsnam )
      {
        oldsnam = snam ;
      }
      else
      {
        stationArr.splice(i, 1);
        i--;
      }
    }
    //alert ("RDBS done for genre " + genre + " with presets: " + stationArr.length);
    callback(id, genre, true);
   }
  }
  xhr.open ( "GET", theUrl ) ;
  resultTimeout = setTimeout(callback, timeout, id, genre, false);
  xhr.send() ;
 }

 function setStation ( inx )
 {
  var table = document.getElementById('stationsTable') ;
  var snam ;
  var theUrl ;

  snam = stationArr[inx].url_resolved ;
  snam = snam.substr ( 7 ) ;
  theUrl = "/?station=" + snam + "&version=" + Math.random() ;
  //table.rows[inx].cells[0].style.backgroundColor = "#333333" 
  xhr = new XMLHttpRequest() ;
  xhr.onreadystatechange = function() 
  {
   if ( this.readyState == XMLHttpRequest.DONE )
   {
     resultstr.value = xhr.responseText ;
   }
  }
  xhr.open ( "GET", theUrl ) ;
  xhr.send() ;
 }
   listDir() ;
</script>

)=====" ;