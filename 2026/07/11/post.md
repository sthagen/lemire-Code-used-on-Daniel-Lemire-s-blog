# X just gave us an interface that AI agents can use. I pointed it at my own posts.

I have been on X for a long time. Like most people who post regularly, I have a gut feeling for what might interest people. I post in the morning. Longer posts seem to do better.

But gut feelings are not measurements. And until recently, digging into your own posting data meant either clicking around the web UI or writing custom scripts. Neither is particularly friendly when you want to ask *ad hoc* questions with an AI assistant.

X recently launched hosted [MCP](https://modelcontextprotocol.io/) servers: official endpoints that AI tools can connect to. MCP is a protocol for plugging tools into language models: the model can search posts, manage bookmarks, fetch trends, and so on. In practice, I connected an AI coding agent to the X MCP server and simply started asking questions about my account.


I spent a session exploring about two months of my own activity. Here is what I found interesting.


Over roughly sixty days (mid-May through mid-July 2026), I published on the order of 435 posts that were not pure retweets of other people—mostly a mix of original posts, replies, and a few X Articles. The agent pulled them through the MCP tools, kept the public metrics (likes, views, reposts), and ran simple analyses.


I asked for every post to be binned by local hour of day (America/Toronto, Eastern time), and for each hour: how many posts, and the min / median / max view count.

My posting is heavily skewed toward the morning:

| Local hour | Posts | Median views |
|------------|------:|-------------:|
| 08:00–08:59 | 45 | 454 |
| 09:00–09:59 | 58 | 1,067 |
| 10:00–10:59 | 30 | 194 |
| 11:00–11:59 | 42 | 284 |

The 9 a.m. hour is both my busiest and, among busy hours, my strongest by median views. The overall median across all hours was only about 188 views, so most of what I write is quiet. The distribution is heavy-tailed: a few posts get tens or hundreds of thousands of impressions; the rest are background noise.

I then binned posts by character length in steps of 25 characters (using the text as returned by the API, including short `t.co` URLs).

The bulk of my writing is short—often a reply of a few dozen characters:

| Characters | Posts | Median likes | Max likes |
|-----------:|------:|-------------:|----------:|
| 0–25 | 44 | 1 | 195 |
| 25–50 | 87 | 1 | 60 |
| 50–75 | 69 | 0 | 98 |
| 75–100 | 51 | 1 | 61 |
| 100–125 | 33 | 1 | 32 |
| … | | | |
| 175–200 | 12 | 5 | 58 |
| 200–225 | 17 | 4 | 456 |
| 275–300 | 19 | 4 | 385 |
| 300–325 | 48 | 46.5 | 470 |

Under about 175 characters, the median stays at zero or one like. Around full-length posts (roughly the old 280-character regime and a bit beyond), engagement jumps. The 300–325 character band is where a large fraction of my “serious” posts live, and the median likes there are an order of magnitude higher than for short replie

I also asked the AI to identify the posts that had the most likes, the following types of posts were liked:

- AI vs. “experts” claiming models are nowhere near human intelligence
- Go adding SIMD-style data-parallelism to the standard library
- SIMD-accelerated data processing talks and library notes (JSON, string→integer maps, vulnerability-report fatigue)
- Nvidia hardware, university AI-cheating, C++ contracts


The interesting part is the workflow. I did not export a CSV by hand and open Excel. I asked an agent, connected to X’s MCP server. If AI agents can do this for one account’s metrics, they can do it for bug trackers,  logs, paper drafts, and codebases.
