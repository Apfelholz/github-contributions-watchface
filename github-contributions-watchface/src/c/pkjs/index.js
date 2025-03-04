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
      var contributions = Array(7).fill().map(() => Array(7).fill(0));
      var now = new Date();
      var oneDay = 24 * 60 * 60 * 1000;

      events.forEach(function(event) {
        if (event.type === 'PushEvent') {
          var eventDate = new Date(event.created_at);
          var diffDays = Math.floor((now - eventDate) / oneDay);
          if (diffDays < 49) { // Only consider events from the last 7 weeks
            var week = Math.floor(diffDays / 7);
            var day = diffDays % 7;
            contributions[week][day] += event.payload.commits.length;
          }
        }
      });

      console.log('Contributions for the last 7 weeks:', contributions);

      // Flatten the contributions array
      var flattenedContributions = [].concat.apply([], contributions);

      // Create a dictionary to send the flattened array
      var dict = {};
      for (var i = 0; i < flattenedContributions.length; i++) {
        dict[KEY_CONTRIBUTIONS + i] = flattenedContributions[i];
      }

      Pebble.sendAppMessage(dict);
    })
    .catch(function(error) {
      console.error('Error fetching GitHub contributions:', error);
    });
}