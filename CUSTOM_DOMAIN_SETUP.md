# Custom Domain Setup for Birds of Play

## Recommended Domain: `birds.beuxbunk.com`

This guide will help you set up a custom subdomain for your Birds of Play project, similar to how `launch.beuxbunk.com` works for your AI Game Generator.

---

## Step 1: Add Domain to Vercel

1. Go to your Vercel dashboard: https://vercel.com/dashboard
2. Click on your `birds-of-play` project
3. Go to **Settings** → **Domains**
4. Click **Add Domain**
5. Enter: `birds.beuxbunk.com`
6. Click **Add**

Vercel will show you the DNS records you need to configure.

---

## Step 2: Configure DNS Records

You'll need to add DNS records in your domain registrar (where you bought `beuxbunk.com`).

### Option A: If using Cloudflare (recommended)

1. Log in to Cloudflare Dashboard
2. Select your `beuxbunk.com` domain
3. Go to **DNS** → **Records**
4. Click **Add record**
5. Add a **CNAME** record:
   - **Type**: CNAME
   - **Name**: `birds` (this creates `birds.beuxbunk.com`)
   - **Target**: `cname.vercel-dns.com`
   - **Proxy status**: DNS only (grey cloud) ⚠️ Important!
   - **TTL**: Auto
6. Click **Save**

### Option B: If using another DNS provider

Add a **CNAME** record:
- **Host/Name**: `birds`
- **Value/Target**: `cname.vercel-dns.com`
- **TTL**: 3600 (or default)

---

## Step 3: Verify Domain in Vercel

1. Back in Vercel, wait a few minutes for DNS to propagate
2. Vercel will automatically verify the domain
3. Once verified, you'll see a green checkmark ✅
4. Vercel will automatically provision an SSL certificate

---

## Step 4: Update Portfolio Link

Once the domain is working, update your portfolio to use the new URL:

```typescript
// In beuxbunk-portfolio/src/components/Projects.tsx
{
  id: 6,
  title: 'Birds of Play',
  description: '...',
  liveDemo: 'https://birds.beuxbunk.com',  // ← Update this
  ...
}
```

---

## Alternative Domain Names

If you prefer a different subdomain:

- `birdsofplay.beuxbunk.com` (more explicit)
- `motion.beuxbunk.com` (focuses on motion detection)
- `dbscan.beuxbunk.com` (highlights the algorithm)
- `birds.beuxbunk.com` (shorter, recommended)

Just replace `birds` with your preferred name in the DNS setup.

---

## Verification & Testing

### Check DNS Propagation
```bash
# Check if DNS is configured correctly
dig birds.beuxbunk.com

# Should show CNAME record pointing to vercel-dns.com
```

### Test the Domain
1. Wait 5-10 minutes after adding DNS record
2. Visit `https://birds.beuxbunk.com`
3. Should redirect to your Birds of Play demo
4. SSL certificate should be active (https with padlock)

---

## Troubleshooting

### Domain not working after 10 minutes
- **Check DNS**: Verify CNAME is set to `cname.vercel-dns.com`
- **Cloudflare users**: Make sure proxy is OFF (grey cloud)
- **Clear cache**: Try incognito/private browsing mode
- **Check Vercel**: Ensure domain shows as verified in Vercel dashboard

### SSL Certificate Issues
- Vercel automatically provisions SSL via Let's Encrypt
- Can take 5-10 minutes after domain verification
- If stuck, try removing and re-adding the domain in Vercel

### "This site can't be reached"
- DNS hasn't propagated yet (can take up to 24 hours, usually 5-10 min)
- Check DNS with: `nslookup birds.beuxbunk.com`
- Try different network (mobile data vs WiFi)

---

## Current Setup Summary

**Before:**
- URL: `https://birds-of-play.vercel.app`
- Hard to remember and share

**After:**
- URL: `https://birds.beuxbunk.com`
- Professional, branded, easy to remember
- Matches your portfolio domain structure
- Similar to `launch.beuxbunk.com` for AI Game Generator

---

## Quick Reference

| Setting | Value |
|---------|-------|
| **Subdomain** | `birds.beuxbunk.com` |
| **DNS Type** | CNAME |
| **DNS Target** | `cname.vercel-dns.com` |
| **Cloudflare Proxy** | OFF (grey cloud) |
| **SSL** | Auto (via Vercel) |
| **Propagation Time** | 5-10 minutes |

---

## Next Steps After Domain is Live

1. ✅ Update portfolio link to `birds.beuxbunk.com`
2. ✅ Test all pages (Motion, Objects, Clustering, Fine-tuning)
3. ✅ Verify GIF and images load correctly
4. ✅ Share the new URL on LinkedIn/resume/GitHub

---

## Notes

- Once configured, Vercel automatically handles all traffic
- SSL certificates auto-renew
- No additional configuration needed
- The `.vercel.app` URL will still work as a fallback
- You can add multiple domains if needed (e.g., both `birds` and `birdsofplay`)

