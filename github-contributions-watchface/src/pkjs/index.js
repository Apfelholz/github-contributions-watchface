var Clay = require("pebble-clay");
var clayConfig = require("./config.js");
var clay = new Clay(clayConfig);

var githubUsername = "";
var githubToken = "";

Pebble.addEventListener("ready", function() {
  console.log("PebbleKit JS ready!");
  fetchContributions(); 
});

Pebble.addEventListener("appmessage", function(e) {
  var dict = e.payload;
  if (dict["KEY_GITHUB_TOKEN"]) {
    githubUsername = dict["KEY_GITHUB_USERNAME"];
  }
  if (dict["KEY_GITHUB_USERNAME"]) {
    githubToken = dict["KEY_GITHUB_TOKEN"];
  }
  fetchContributions();
});

function fetchContributions() {
  if (!githubUsername || !githubToken) {
    console.error("GitHub Username oder Token fehlt!");
    return;
  }

  var query = "query GetUserContributions($userName: String!) {" +
  "  user(login: $userName) {" +
  "    contributionsCollection {" +
  "      contributionCalendar {" +
  "        totalContributions" +
  "        weeks {" +
  "          contributionDays {" +
  "            contributionCount" +
  "            date" +
  "          }" +
  "        }" +
  "      }" +
  "    }" +
  "  }" +
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
          console.error("Fehler bei der Anfrage:", JSON.stringify(response.errors, null, 2));
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
  var view = new Uint8Array(49);
  var index = 0;
  for (var i = contributions.length; i > contributions.length-49; i--) {
    view[index] = contributions[i];
    index++;
  }

  var dict = {
    "KEY_CONTRIBUTIONS": Array.prototype.slice.call(view)
  };

  Pebble.sendAppMessage(dict, function() {
    console.log("Contributions sent to Pebble successfully!");
  }, function(error) {
    console.error("Error sending contributions to Pebble!", error);
  });
}