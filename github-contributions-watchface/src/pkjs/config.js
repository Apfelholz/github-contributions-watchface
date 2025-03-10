module.exports = [
  {
    "type": "heading",
    "defaultValue": "Api Configuration"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Github"
      },
      {
        "type": "input",
        "messageKey": "KEY_GITHUB_USERNAME",
        "defaultValue": "",
        "label": "Username",
        "attributes": {
          "placeholder": "Username"
        }
      },
      {
        "type": "input",
        "messageKey": "KEY_GITHUB_TOKEN",
        "defaultValue": "",
        "label": "Github Token",
        "description": "You can generate a token at <a href='https://github.com/settings/tokens'>here</a>. The token needs the 'read:user' permission.",
        "attributes": {
          "placeholder": "Token"
        }
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];

