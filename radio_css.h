// radio.css file in raw data format for PROGMEM
//
const char radio_css[] PROGMEM = R"=====(
body {
   background-color: lightblue;
//   font-family: Arial, Helvetica, sans-serif;
}

* {
        border-bottom-left-radius: 1em;
        border-top-right-radius: 1em;
        font-size: 1em;
   }

h1 {
   color: navy;
//   margin-left: 20px;
   font-size: 4em
}

txt3 {
  font-size: 3em;
  padding: 5em 0;
}

ul {
   list-style-type: none;
   margin: 0;
   margin-left: -10px;
   overflow: hidden;
   background-color: #2C3E50;
   position:fixed;
   top:0;
   width:100%;
   z-index:100;
}

li .pull-left {
   float: left;
}

li .pull-right {
   float: right;
}

li a {
   display: block;
   color: white;
   text-align: center;
   padding: 14px 16px;
   text-decoration: none;
}

li a:hover, a.active {
   background-color: #111;
}

label {
  font-size: 2em;
}

.big {
  font-size: 2em;
}

.presetcontainer {
  width = 100%;
  text-align: center;
}

.slidecontainer {
  margin-top: 1em;
  margin-bottom: 1em;
  width = 100%;
  text-align: center;
}

.slider {
  -webkit-appearance: none;
  width: 80%;
  height: 3em;
  background: #d3d3d3;
  outline: none;
  opacity: 0.7;
  -webkit-transition: .2s;
  transition: opacity .2s;
}

.slider.small {
  width: 40%;
}

.slider:hover {
  opacity: 1;
}

.slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 1.5em;
  height: 5em;
  background: #4CAF50;
  cursor: pointer;
}

.slider::-moz-range-thumb {
  width: 1.5em;
  height: 5em;
  background: #4CAF50;
  cursor: pointer;
}

.button {
   width: 12em;
   height: 3em;
   background-color: #128F76;
 //  border: none;
   color: white;
   text-align: center;
   text-decoration: none;
   display: inline-block;
   font-weight: bold;
   font-size: 2em;
   margin: 0.5em;
   cursor: pointer;
//   border-radius: 1.4em;
}
.radiotextcontainer {
  width: 90%;
  margin-top: 2em;
  margin-bottom: 2em;
}

.radiotxt {
  margin-top: 2em;
  margin-bottom: 2em;
  width: 100%;
  height: 4em;
  font-size: 2em;
  color: white;
  background-color: black;
  border-color: white;
  border-style: solid;  
//  border: 1px;
}

.buttonr {background-color: #D62C1A;}

.select {
   width: 80%;
   height: 2.5em;
   padding: 5px;
   background: white;
   font-size: 2em;
   line-height: 1;
   border: 1;
   border-radius: 5px;
   -webkit-border-radius: 5px;
   -moz-border-radius: 5px;
   -webkit-appearance: none;
   border: 1px solid black;
   border-radius: 10px;
}

.selectw {width: 24em}

input[type="text"] {
   width: 24em;
   margin: 0;
   height: 2.5em;
   background: white;
   font-size: 2em;
   appearance: none;
   box-shadow: none;
   border-radius: 5px;
   -webkit-border-radius: 5px;
   -moz-border-radius: 5px;
   -webkit-appearance: none;
   border: 1px solid black;
   border-radius: 10px;
}

input[type="text"]:focus {
  outline: none;
}

input[type=submit] {
  width: 22em;
  height: 3em;
  background-color: #128F76;
  border: none;
  color: white;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 1em;
  margin: 4px 2px;
  cursor: pointer;
  border-radius: 5px;
}

textarea {
  width: 80;
  height: 25;
  border: 1px solid black;
  border-radius: 10px;
  padding: 5px;
  font-family: Courier, "Lucida Console", monospace;
  background-color: white;
  resize: none;
})=====" ;
