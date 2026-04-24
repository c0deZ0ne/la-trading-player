import subprocess
import sys

def run_ssh(cmd):
    password = "S570I59VsnFpC0eIre"
    ssh_cmd = f'sshpass -p "{password}" ssh -o StrictHostKeyChecking=no root@107.172.34.199 "{cmd}"'
    result = subprocess.run(ssh_cmd, shell=True, capture_output=True, text=True)
    return result.stdout, result.stderr

print("Checking IP forwarding...")
stdout, stderr = run_ssh("cat /proc/sys/net/ipv4/ip_forward")
print(f"IP Forward: {stdout.strip()}")

print("\nChecking Public Interface...")
stdout, stderr = run_ssh("ip route get 8.8.8.8 | grep -oP 'dev [^ ]+' | cut -d' ' -f2")
pub_iface = stdout.strip()
print(f"Public Interface: {pub_iface}")

print("\nChecking IPTables Forward Policy...")
stdout, stderr = run_ssh("iptables -L FORWARD -n")
print(stdout)

print("\nChecking NAT table...")
stdout, stderr = run_ssh("iptables -t nat -L POSTROUTING -n")
print(stdout)

print("\nTesting ping to internet from server...")
stdout, stderr = run_ssh("ping -c 1 8.8.8.8")
print(stdout)
