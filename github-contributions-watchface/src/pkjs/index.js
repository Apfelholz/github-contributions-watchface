var Clay = require("pebble-clay");
var clayConfig = require("./config");
var clay = new Clay(clayConfig);

var githubUsername = "Apfelholz";
var githubToken = "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";

Pebble.addEventListener("ready", function() {
  console.log("PebbleKit JS ready!");
  fetchContributions(); 
});

Pebble.addEventListener("appmessage", function(e) {
  console.log("AppMessage received!");
  if (e.payload.KEY_GITHUB_USERNAME) {
    githubUsername = e.payload.KEY_GITHUB_USERNAME;
    console.log("Received GitHub username: " + githubUsername);
  }
  if (e.payload.KEY_GITHUB_TOKEN) {
    githubToken = e.payload.KEY_GITHUB_TOKEN;
    console.log("Received GitHub token: " + githubToken);
  }
  fetchContributions();
});

function fetchContributions() {
  if (!githubUsername || !githubToken) {
    console.error("GitHub Username oder Token fehlt!");
    return;
  }

  var query = "query GetUserContributions($userName: String!) {" +
              "user(login: $userName) {" +
              "contributionsCollection {" +
              "contributionCalendar {" +
              "totalContributions" +
              "weeks {" +
              "contributionDays {" +
              "contributionCount" +
              "date" +
              "}" +
              "}" +
              "}" +
              "}" +
              "}";

  var variables = { userName: githubUsername };

  var xhr = new XMLHttpRequest();
  xhr.open("POST", "https://api.github.com/graphql", true);
  xhr.setRequestHeader("Content-Type", "application/json");
  xhr.setRequestHeader("Authorization", "Bearer " + githubToken);

  xhr.onreadystatechange = function() {
    if (xhr.readyState === XMLHttpRequest.DONE) {
      if (xhr.status === 200) {
        var response = JSON.parse(xhr.responseText);
        if (response.errors) {
          console.error("Fehler bei der Anfrage:", response.errors);
          return;
        }

        var weeks = [];
        if (response.data && response.data.user && response.data.user.contributionsCollection && response.data.user.contributionsCollection.contributionCalendar) {
          weeks = response.data.user.contributionsCollection.contributionCalendar.weeks || [];
        }

        var contributions = [];
        for (var i = 0; i < weeks.length; i++) {
          var week = weeks[i];
          if (week.contributionDays) {
            for (var j = 0; j < week.contributionDays.length; j++) {
              contributions.push(week.contributionDays[j].contributionCount);
            }
          }
        }

        sendContributions(contributions);
      } else {
        console.error("Fehler beim Abrufen der Beiträge:", xhr.statusText);
      }
    }
  };

  xhr.send(JSON.stringify({ query: query, variables: variables }));
}

function sendContributions(contributions) {
  contributions = contributions || []; // Fallback auf leeres Array, wenn keine Beiträge übergeben wurden

  var buffer = new ArrayBuffer(contributions.length);
  var view = new Uint8Array(buffer);
  for (var i = 0; i < contributions.length; i++) {
    view[i] = contributions[i];
  }

  var dict = {
    "KEY_CONTRIBUTIONS": Array.prototype.slice.call(view)
  };

  Pebble.sendAppMessage(dict, function() {
    console.log("Contributions sent to Pebble successfully!" + JSON.stringify(dict));
  }, function(error) {
    console.log("Error sending contributions to Pebble!", error);
  });
}
