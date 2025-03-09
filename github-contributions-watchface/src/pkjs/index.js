var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig);

var githubUsername = '';
var githubToken = '';

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  sendContributions();
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received!');
  if (e.payload.KEY_GITHUB_USERNAME) {
    githubUsername = e.payload.KEY_GITHUB_USERNAME;
    console.log('Received GitHub username: ' + githubUsername);
  }
  if (e.payload.KEY_GITHUB_TOKEN) {
    githubToken = e.payload.KEY_GITHUB_TOKEN;
    console.log('Received GitHub token: ' + githubToken);
  }
  sendContributions();
});

function fetchContributions() {
  const query = `
    query GetUserContributions($userName: String!) {
      user(login: $userName) {
        contributionsCollection {
          contributionCalendar {
            totalContributions
            weeks {
              contributionDays {
                contributionCount
                date
              }
            }
          }
        }
      }
    }
  `;

  const variables = { userName: githubUsername };

  const xhr = new XMLHttpRequest();
  xhr.open('POST', 'https://api.github.com/graphql', true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.setRequestHeader('Authorization', `Bearer ${githubToken}`);

  xhr.onreadystatechange = function() {
    if (xhr.readyState === XMLHttpRequest.DONE) {
      if (xhr.status === 200) {
        const response = JSON.parse(xhr.responseText);
        if (response.errors) {
          console.error('Fehler bei der Anfrage:', response.errors);
          return;
        }

        const weeks = response.data.user.contributionsCollection.contributionCalendar.weeks;
        const contributions = weeks.flatMap(week => week.contributionDays.map(day => day.contributionCount));

        sendContributions(contributions);
      } else {
        console.error('Fehler beim Abrufen der Beiträge:', xhr.statusText);
      }
    }
  };

  xhr.send(JSON.stringify({ query, variables }));
}

function sendContributions(contributions) {
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