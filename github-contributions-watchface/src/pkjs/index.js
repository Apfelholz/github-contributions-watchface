
Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  sendContributions();
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received!');
  sendContributions();
});

function sendContributions() {
  const contributions = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 
    40, 41, 42, 43, 44, 45, 46, 47, 48
  ];

  var buffer = new ArrayBuffer(contributions.length);
  var view = new Uint8Array(buffer);
  for (var i = 0; i < contributions.length; i++) {
    view[i] = contributions[i];
  }

  var dict = {
    'KEY_CONTRIBUTIONS': Array.prototype.slice.call(view)
  };

  Pebble.sendAppMessage(dict, function() {
    console.log('Contributions sent to Pebble successfully!' + JSON.stringify(dict));
  }, function(error) {
    console.log('Error sending contributions to Pebble!', error);
  });
}
