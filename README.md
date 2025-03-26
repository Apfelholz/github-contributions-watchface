# GitHub Contributions Watchface

A Pebble watchface that dynamically changes its background color based on your GitHub contributions.

## Features

- Fetches GitHub contributions every five minutes.
- Background color changes from dark gray (no contributions) to various shades of green (more contributions) to white (over 30 contributions).
- Black and white platforms have two different shading options -- Bayer dithering and stripes.

## Screenshots

![aplite](aplite.png)
![basalt](basalt.png)
![diorite](diorite.png)

## Installation

The watchface is available on the Pebble app store: [GitHub Contributions Watchface](https://apps.rebble.io/en_US/application/67ce34d2b7a0230391426cbe).

## Configuration

1. Open the Pebble app on your phone.
2. Navigate to the settings of the GitHub Contributions Watchface.
3. Enter your GitHub username and a [personal access token](https://github.com/settings/tokens) with the `read:user` permission.

## License

This project is licensed under the Apache License 2.0. See the [`LICENSE`](LICENSE) file for details.
