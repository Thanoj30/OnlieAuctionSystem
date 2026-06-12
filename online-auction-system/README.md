# Online Auction Management System

A beginner-friendly, intermediate-level Online Auction Management System built with a C++ Crow backend and a simple HTML/CSS frontend.

The project demonstrates user registration, login, item listing, bidding, auction closing, winner declaration, and file-based persistence without adding a database or complex dependencies.

## Technologies Used

- C++17
- Crow C++ web framework
- CMake
- HTML
- CSS
- Text files for simple storage

## Features

- User registration and login
- Logout using a simple cookie clear route
- Dashboard for logged-in users
- Add auction items from the dashboard
- Auto-generated item IDs
- Auction page with responsive item cards
- Place bids on open auction items
- Prevent sellers from bidding on their own items
- Reject bids lower than or equal to the current highest bid
- Close auction items as the seller
- Declare winner from the highest bidder
- Show `No winner` when an auction closes without bids
- Store users, items, and bids in text files

## Project Structure

```text
online-auction-system/
  main.cpp
  CMakeLists.txt
  users.txt
  items.txt
  bids.txt

  templates/
    index.html
    login.html
    register.html
    dashboard.html
    auction.html

  static/
    style.css

  README.md
```

## Project Architecture

The application uses a small MVC-style structure:

- `main.cpp`: Crow routes, form handling, validation, file handling, and simple model structs.
- `templates/`: HTML pages served by the backend.
- `static/style.css`: shared UI styling.
- `users.txt`: registered user data.
- `items.txt`: auction item data, current bid, status, and winner.
- `bids.txt`: bid history.

Data format examples:

```text
users.txt:
username,password,full name

items.txt:
item_id|item_name|description|starting_price|seller_username|highest_bid|highest_bidder|status|winner

bids.txt:
bid_id|item_id|bidder_username|bid_amount
```

## Routes

- `GET /` home page
- `GET /login` login page
- `POST /login` validate login
- `GET /logout` clear login cookie
- `GET /register` registration page
- `POST /register` create user
- `GET /dashboard` dashboard for logged-in users
- `POST /add-item` add auction item
- `GET /auction` view auction items
- `POST /place-bid` place a bid
- `POST /close-auction` close an auction as seller
- `GET /static/style.css` serve CSS

## Dependencies

On macOS, install Apple command line tools if needed:

```bash
xcode-select --install
```

Install CMake and Crow:

```bash
brew install cmake
brew install crow
```

If compilation complains about `asio.hpp`, install Asio:

```bash
brew install asio
```

## Build

From the `online-auction-system` folder:

```bash
cmake -S . -B build
cmake --build build
```

## Run

From the `online-auction-system` folder:

```bash
./build/online_auction
```

Open:

```text
http://localhost:18080
```

The server binds to `127.0.0.1:18080`.


This project is a C++ web application using the Crow framework. I used Crow routes to handle page requests and form submissions. The frontend is built with HTML templates and one shared CSS file.

For storage, I used text files to keep the project beginner-friendly. Users are stored in `users.txt`, auction items in `items.txt`, and bid history in `bids.txt`. The backend reads these files into simple C++ structs such as `User`, `AuctionItem`, and `Bid`, which demonstrates basic OOP-style data modeling.

The application validates user actions before updating files. For example, duplicate usernames are blocked, a bid must be greater than the current highest bid, sellers cannot bid on their own items, and closed auctions reject new bids. When an auction is closed, the highest bidder becomes the winner; if there are no bids, the winner is marked as `No winner`.

Searching is used to find users and items by username or item ID. Sorting is used to display open auctions before closed auctions. File handling is used for reading, appending, and rewriting records.

This design is intentionally simple. In a production version, the text files could be replaced with a database, passwords would be hashed, and sessions would be handled more securely.

## Current Limitations

- Passwords are stored as plain text for learning purposes.
- Cookie-based login is simplified.
- Text files are used instead of a database.
- No admin role is implemented.
- No auction timer is implemented.
