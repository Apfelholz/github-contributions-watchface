Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  fetchGitHubContributions('Apfelholz');
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received!');
  fetchGitHubContributions('Apfelholz');
});

function fetchGitHubContributions(username) {
  var url = 'https://api.github.com/users/' + username + '/events';
  fetch(url)
    .then(function(response) {
      return response.json();
    })
    .then(function(events) {
      var contributions = 0;
      events.forEach(function(event) {
        if (event.type === 'PushEvent') {
          contributions += event.payload.commits.length;
        }
      });
      console.log('Total contributions: ' + contributions);
      Pebble.sendAppMessage({ 'KEY_CONTRIBUTIONS': contributions });
    })
    .catch(function(error) {
      console.error('Error fetching GitHub contributions:', error);
    });
}