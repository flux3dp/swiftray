import subprocess
import re
import argparse
from datetime import datetime
from lxml import etree        # For parsing source xml and insert element
# NOTE: xml.etree.ElementTree doesn't support prettier print until Python 3.9
#       https://stackoverflow.com/questions/749796/pretty-printing-xml-in-python 
from xml.dom import minidom   # For indenting (make prettier) the result xml

def generate_appcast(is_ed25519, signature_file, release_ver, src_appcast_file):
    if is_ed25519:
        # Generate signature
        result_pattern = r'sparkle:edSignature="(?P<signature>[a-zA-Z0-9=+\/]{88})" length="(?P<length>[\d]+)"'
        f = open(signature_file)
        # Parse sign_update output into usable variables
        m = re.match(result_pattern, f.read().strip())
        signature = m.group('signature')
        len = m.group('length')
    else:
        f = open(signature_file)
        signature = f.read().replace("\n", "")
        len = '0'

    # Get publish date
    now = datetime.now()
    pub_date = now.strftime("%a, %d %b %Y %H:%M:%S +0000")

    # Parse source appcast.xml
    xml_tree = etree.parse(src_appcast_file)
    rss_root = xml_tree.getroot()
    sparkle_namespace = rss_root.nsmap['sparkle']
    xsparkle = f"{{{sparkle_namespace}}}"
    channel = next(rss_root.iter("channel"))
    # Create new update item
    xitem = etree.SubElement(channel, "item")
    xitem_title = etree.SubElement(xitem, "title")
    xitem_title.text = release_ver
    xitem_pubdate = etree.SubElement(xitem, "pubDate")
    xitem_pubdate.text = pub_date
    xitem_link = etree.SubElement(xitem, "link")
    xitem_link.text = "https://swiftray.io/"
    xitem_sparkle_version = etree.SubElement(xitem, xsparkle + "version")
    xitem_sparkle_version.text = release_ver
    xitem_sparkle_short_version = etree.SubElement(xitem, xsparkle + "shortVersionString")
    xitem_sparkle_short_version.text = release_ver
    #xitem_sparkle_min_system_ver = etree.SubElement(xitem, xsparkle + "minimumSystemVersion")
    #xitem_sparkle_min_system_ver.text = "10.13"
    xitem_sparkle_release_link = etree.SubElement(xitem, xsparkle + "releaseNotesLink")
    xitem_sparkle_release_link.text = "https://swiftray.s3.ap-northeast-1.amazonaws.com/changelog/swiftray_" + release_ver + ".html"
    xitem_enclosure = etree.SubElement(xitem, "enclosure")
    if is_ed25519: # For macOS
        xitem_enclosure.set('url', "https://swiftray.s3.ap-northeast-1.amazonaws.com/mac/swiftray_" + release_ver + ".dmg")
        xitem_enclosure.set(xsparkle + "edSignature", signature)
        xitem_enclosure.set('length', len)
    else: # For Windows
        xitem_enclosure.set('url', "https://swiftray.s3.ap-northeast-1.amazonaws.com/win/swiftray_" + release_ver + ".exe")
        xitem_enclosure.set(xsparkle + "dsaSignature", signature)
        xitem_enclosure.set('length', len)
    xitem_enclosure.set('type', "application/octet-stream")
    #etree.tostring(xitem, pretty_print=True)
    #etree.tostring(rss_root, pretty_print=True)

    return xml_tree


def main():
    """ Parse source appcast file and insert new release update item into it
        and write to the result file
    """
    parser = argparse.ArgumentParser(description='Sparkle appcast generator')
    group_key_type = parser.add_mutually_exclusive_group()
    group_key_type.add_argument('--dsa', action='store_false')    # For WinSparkle (old version of encrypt)
    group_key_type.add_argument('--ed25519', action='store_true') # For Sparkle    (new version of encrypt)
    parser.add_argument('--signature', dest='signature', type=str, required=True,
                        help='Path to the signature result')
    parser.add_argument('--release_ver', dest='release_ver', type=str, required=True,
                        help='Release version of Swiftray')
    parser.add_argument('--src_appcast', dest='src_appcast', type=str, required=True,
                        help='Path to the source appcast file')
    options = parser.parse_args()

    xml_tree = generate_appcast(options.ed25519,
                      options.signature,
                      options.release_ver,
                      options.src_appcast)

    # Write new appcast xml
    dom = minidom.parseString(etree.tostring(xml_tree.getroot(), pretty_print=True))
    print(dom.toprettyxml(indent='\t', newl='\n'))
    with open(options.src_appcast, 'w') as f:
        f.write(dom.toprettyxml(indent='\t', newl='\n'))
    

if __name__ == "__main__":
    main()