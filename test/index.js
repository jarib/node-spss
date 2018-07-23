const spss = require('bindings')('spss');
const fs = require('fs-extra');

console.log(JSON.stringify(
    spss.convert(process.argv[2] || `${__dirname}/testdata.sav`)
, null, 2));

