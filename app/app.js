const express = require('express');
const path = require('path');

const app = express();
app.use(express.static(path.join(__dirname, 'build')));
app.get('/*', function (req, res) {
  res.setHeader('Cache-Control', 'private, no-cache, no-store, must-revalidate');
  res.setHeader('Expires', '-1');
  res.setHeader('Pragma', 'no-cache');
  res.sendFile(path.join(__dirname, 'build', 'index.html'));
});
app.listen(process.env.PORT || 80);
