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

  // Testdaten
  const contributions = [
    [0, 1, 2, 3, 4, 5, 6],
    [7, 8, 9, 10, 11, 12, 13],
    [14, 15, 16, 17, 18, 19, 20],
    [0, 1, 2, 3, 4, 5, 6],
    [7, 8, 9, 10, 11, 12, 13],
    [14, 15, 16, 17, 18, 19, 20],
    [0, 1, 2, 3, 4, 5, 6]
  ];

  // Wandelt das 2D-Array in ein Uint8Array um
  const data = new Uint8Array(7 * 7 * 4);  // 7x7 Tage, 4 Bytes pro Wert

  let index = 0;
  for (let week = 0; week < 7; week++) {
    for (let day = 0; day < 7; day++) {
      const contribution = contributions[week][day];

      // Packe den Beitrag (32-Bit Integer) in die Binärdaten (4 Bytes)
      data[index] = (contribution >> 24) & 0xFF;      // Höchstes Byte
      data[index + 1] = (contribution >> 16) & 0xFF;  // Zweites Byte
      data[index + 2] = (contribution >> 8) & 0xFF;   // Drittens Byte
      data[index + 3] = contribution & 0xFF;           // Niedrigstes Byte

      index += 4;
    }
  }

  // Senden der Binärdaten an die Pebble App
  var dict = {
    'Data': data.buffer
  };

  Pebble.sendAppMessage(dict, function() {
    console.log("Contributions sent successfully!");
  }, function(error) {
    console.log("Error sending data: " + error);
  });
}