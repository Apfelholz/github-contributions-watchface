Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  sendContributions();
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received!');
  sendContributions();
});

function fetchContributions() {
  const query = 
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
  ;

  const variables = {
    userName: "Apfelholz" // Benutzername hier einsetzen
  };

  const headers = {
    "Content-Type": "application/json",
    "Authorization": "" // Dein Token
  };

  fetch('https://api.github.com/graphql', {
    method: 'POST',
    headers: headers,
    body: JSON.stringify({ query, variables })
  })
  .then(response => response.json())
  .then(data => {
    if (data.errors) {
      console.error('Fehler bei der Anfrage:', data.errors);
      return;
    }

    const weeks = data.data.user.contributionsCollection.contributionCalendar.weeks;
    const contributions = weeks.slice(0, 7).map(week => week.contributionDays.map(day => day.contributionCount));

    sendContributions(contributions) // Sendet die Daten an Pebble App
  })
  .catch(error => console.error('Fehler beim Abrufen der Beiträge:', error));
}

function sendContributions() {
  const contributions = [
    [0, 1, 2, 3, 4, 5, 6],
    [7, 8, 9, 10, 11, 12, 13],
    [14, 15, 16, 17, 18, 19, 20],
    [21, 22, 23, 24, 25, 26, 27],
    [28, 29, 30, 31, 32, 33, 34],
    [35, 36, 37, 38, 39, 40, 41],
    [42, 43, 44, 45, 46, 47, 48]
  ];

  const data = new Uint8Array(7 * 7 * 4);
  let index = 0;
  for (let week = 0; week < 7; week++) {
    for (let day = 0; day < 7; day++) {
      const contribution = contributions[week][day];
      data[index] = (contribution >> 24) & 0xFF;
      data[index + 1] = (contribution >> 16) & 0xFF;
      data[index + 2] = (contribution >> 8) & 0xFF;
      data[index + 3] = contribution & 0xFF;
      index += 4;
    }
  }

  Pebble.sendAppMessage({ "Data": data.buffer },
    function() { console.log("Contributions sent successfully!"); },
    function(error) { console.log("Error sending data: " + JSON.stringify(error)); }
  );
}
