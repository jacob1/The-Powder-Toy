import urllib.error
import urllib.request
import io
import json
import os
from PIL import Image

# Folder names
github_avatars = "github_avatars"
tpt_avatars = "tpt_avatars"

def get_url(url : str) -> bytes | None:
	try:
		req = urllib.request.Request(url)
		data = urllib.request.urlopen(req, timeout=10)
		page = data.read()

		return page
	except urllib.error.URLError as e:
		print(f"{url} - {e}")
		return None

def get_avatar(avatar_url : str) -> Image:
	data = get_url(avatar_url)
	image = Image.open(io.BytesIO(data))
	# Some avatars are 40x40 in-game, but it looks better on most avatars if TPT downscales from 64->40 instead of PIL
	resized = image.resize((64, 64), Image.Resampling.LANCZOS)
	return resized

def mkdir(directory : str) -> None:
	try:
		os.mkdir(directory)
	except FileExistsError:
		return

def fetch_gh_contributors() -> list[any]:
	page = 1
	ret = []
	while True:
		data = get_url(f"https://api.github.com/repos/The-Powder-Toy/The-Powder-Toy/contributors?page={page}")
		contributors = json.loads(data)
		if not len(contributors):
			break
		ret.extend(contributors)
		page = page + 1

	return ret

def fetch_github_avatars(contributors : list[any]) -> None:
	mkdir(github_avatars)

	for contributor in contributors:
		username = contributor["login"]
		avatar_url = contributor["avatar_url"]
		avatar = get_avatar(avatar_url)
		avatar.save(f"{github_avatars}/{username}.png", "PNG")

	with open(f"{github_avatars}/github_avatars.txt", "w") as f:
		for contributor in contributors:
			f.write(contributor["login"])
			f.write("\n")

def fetch_tpt_avatars(original_credits : list[dict[str, str]], mods : list[dict[str, str]]) -> None:
	mkdir(tpt_avatars)

	set1 = [ item["username"] for item in original_credits ]
	set2 = [ item["username"] for item in mods ]
	usernames = set(set1).union(set2)

	for username in usernames:
		url = f"https://static.powdertoy.co.uk/avatars/{username}.256.png"
		avatar = get_avatar(url)
		avatar.save(f"{tpt_avatars}/{username}.png", "PNG")

	with open(f"{tpt_avatars}/tpt_avatars.txt", "w") as f:
		for username in usernames:
			f.write(username)
			f.write("\n")

def get_github_json(github_contributors : list[any]) -> list[dict[str, str]]:
	tpt_mapping = get_gh_mapping()

	ret = []
	for contributor in github_contributors:
		gh_username = contributor["login"]
		tpt_username = tpt_mapping[gh_username] if gh_username in tpt_mapping else None
		if tpt_username:
			ret.append({"gh":f"{gh_username}", "tpt":f"{tpt_username}"})
		else:
			ret.append({"gh":f"{gh_username}"})

	return ret

def get_gh_mapping() -> dict[str, str]:
	"""Returns map of GitHub usernames to TPT usernames. List created by hand."""

	return {
		"simtr" : "Simon",
		"jacob1" : "jacob1",
		"LBPHacker" : "LBPHacker",
		"jacksonmj" : "jacksonmj",
		"pilihp64" : "cracker64",
		"mniip" : "mniip",
		"savask" : "savask",
		"NoH" : "Felix",
		"triclops200" : "triclops200",
		"SuperDoxin" : "doxin",
		"zc00gii" : "zc00gii",
		"krawthekrow" : "mark2222",
		"wolfy1339" : "wolfy1339",
		"catsoften" : "catsoften",
		"QuanTech0" : "QuanTech",
		"Catelite" : "Catelite",
		"antb" : "Xenocide",
		"cracker1000" : "Cracker1000",
		"moonheart08" : "moonheart08",
		"Huulivoide" : "Huulivoide",
		"ntoskrnl11" : "ntoskrnl",
		"me4502" : "me4502",
		"tridiaq" : "arK",
		"perssphere07" : "perssphere07",
		"jbot-42" : "jBot-42",
		"C7C8" : "Sourec",
		"boxmein" : "boxmein",
		"ief015" : "ief015",
		"nixls" : "nikigameplay",
		"kroq-gar78" : "kroq-gar78",
		"BlueSyncLine" : "SopaXorzTaker",
		"ssccsscc" : "ssccsscc",
		"gamax92" : "BlueAmulet",
		"Ristovski" : "Ristovski",
		"nucular" : "nucular",
		"jombo23" : "jombo23",
		"Bowserinator" : "Bowserinator",
		"VelocityRa" : "VelocityRa",
		"nunom27" : "nunom",
		"jebbyk" : "zaicev9797",
		"grufkork" : "Grufkork",
		"RCAProduction" : "RCAProduction",
		"connor-create" : "cj646464",
		"avevad" : "avevad",
		"Mrprocom" : "Mrprocom",
		"Vgr255" : "Vgr255",
		"n1kolasM" : "n1kolasM",
		"cppxor2arr" : "RecursiveOverflow",
		"handicraftsman" : "handicraftsman",
		"china-richway2" : "china-richway2",
		"meyer9" : "jmeyer2k",
		"SilentSpud" : "reap3r119",
		"um3k" : "um3k",
		"yangbowen" : "yangbowen1",
		"Onestay42" : "Onestay",
		"JasonS05" : "JasonS",
		"Ksawi999" : "Ksawi999",
		"Jakav-N" : "Jakav",
		"Rebmiami" : "Rebmiami",
		"Maticzpl" : "Maticzpl",
		"dreness" : "dreness",
		"Departing" : "Sylvi",
	}

def get_orig_json() -> list[dict[str, str]]:
	"""Credits that appeared in intro text in older versions"""

	return [
		{ "username" : "ALark",       "ghUsername" : "",            "realname" : "Stanislaw K Skowronek", "message" : "Designed the original Powder Toy"},
		{ "username" : "Simon",       "ghUsername" : "simtr",       "realname" : "Simon Robertshaw",      "message" : "Wrote the website, current server owner"},
		{ "username" : "savask",      "ghUsername" : "savask",      "realname" : "Skresanov Savely",      "message" : ""},
		{ "username" : "cracker64",   "ghUsername" : "pilihp64",    "realname" : "Pilihp64",              "message" : ""},
		{ "username" : "Catelite",    "ghUsername" : "Catelite",    "realname" : "Catelite",              "message" : ""},
		{ "username" : "triclops200", "ghUsername" : "triclops200", "realname" : "Victoria Hoyle",        "message" : ""},
		{ "username" : "ief015",      "ghUsername" : "ief015",      "realname" : "Nathan Cousins",        "message" : ""},
		{ "username" : "jacksonmj",   "ghUsername" : "jacksonmj",   "realname" : "jacksonmj",             "message" : ""},
		{ "username" : "Felix",       "ghUsername" : "NoH",         "realname" : "Felix Wallin",          "message" : ""},
		{ "username" : "doxin",       "ghUsername" : "SuperDoxin",  "realname" : "Lieuwe Mosch",          "message" : ""},
		{ "username" : "Xenocide",    "ghUsername" : "antb",        "realname" : "Anthony Boot",          "message" : ""},
		{ "username" : "me4502",      "ghUsername" : "me4502",      "realname" : "Me4502",                "message" : ""},
		{ "username" : "MaksProg",    "ghUsername" : "",            "realname" : "MaksProg",              "message" : ""},
		{ "username" : "jacob1",      "ghUsername" : "jacob1",      "realname" : "jacob1",                "message" : ""},
		{ "username" : "mniip",       "ghUsername" : "mniip",       "realname" : "mniip",                 "message" : ""},
		{ "username" : "LBPHacker",   "ghUsername" : "LBPHacker",   "realname" : "LBPHacker",             "message" : ""},
	]

def get_moderator_json() -> list[dict[str, str]]:
	"""Current and former moderators"""

	return [
		{ "username" : "jacob1",      "role" : "Moderator" },
		{ "username" : "LBPHacker",   "role" : "Moderator" },
		{ "username" : "Sylvi",       "role" : "Moderator" },
		{ "username" : "CCl2F2",      "role" : "Moderator" },
		{ "username" : "catsoften",   "role" : "Moderator" },
		{ "username" : "Denderth",    "role" : "Moderator" },
		{ "username" : "Simon",       "role" : "Moderator" },
		{ "username" : "Mrprocom",    "role" : "Moderator" },
		{ "username" : "jacksonmj",   "role" : "Former Staff" },
		{ "username" : "cracker64",   "role" : "Former Staff" },
		{ "username" : "Catelite",    "role" : "Former Staff" },
		{ "username" : "boxmein",     "role" : "Former Staff" },
		{ "username" : "lolzy",       "role" : "Former Staff" },
		{ "username" : "Xenocide",    "role" : "Former Staff" },
		{ "username" : "triclops200", "role" : "Former Staff" },
		{ "username" : "devast8a",    "role" : "Former Staff" },
		{ "username" : "HK6",         "role" : "Former Staff" },
		{ "username" : "FrankBro",    "role" : "Former Staff" },
		{ "username" : "doxin",       "role" : "Former Staff" },
		{ "username" : "ief015",      "role" : "Former Staff" },
		{ "username" : "ad",          "role" : "Former Staff" },
	]

def process() -> any:
	github_contributors = fetch_gh_contributors()

	github = get_github_json(github_contributors)
	orig = get_orig_json()
	mods = get_moderator_json()

	fetch_github_avatars(github_contributors)
	fetch_tpt_avatars(orig, mods)

	data = {
		"GitHub" : github,
		"OrigCredits" : orig,
		"Moderators" : mods,
	}

	with open("credits.json", "w") as f:
		json.dump(data, f)

process()
